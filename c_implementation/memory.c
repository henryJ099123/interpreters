#include <stdlib.h>

#include "memory.h"
#include "vm.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

// seems just to be a wrapper on `realloc`
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    // acquiring *more* memory forces a garbage collection
    // don't want to trigger a GC if we are freeing or shrinking an allocation
    if(newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
    } 
	if(newSize == 0) {
		free(pointer);
		// return NULL ptr? yikes
		return NULL;
	} 

	void* result = realloc(pointer, newSize);
	if(result == NULL) exit(1);
	return result;
} 

void markObject(Obj* object) {
    if(object == NULL) return;
    if(object->isMarked) return; // already've done the work for this object
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*) object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    // mark the object via a field
    object->isMarked = true;

    // uses `realloc` instead of `reallocate` wrapper bc the GC
    // activates via uses of `reallocate`
    // this thing's memory will be managed explicitly
    if(vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**) realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);

        // must handle allocation failure
        if(vm.grayStack == NULL) exit(1);
    } 

    // add the marked object to the gray array
    vm.grayStack[vm.grayCount++] = object;
} 

void markValue(Value value) {
    // numbers, booleans, nil are stored inline in Value structs
    // and require no heap allocation
    // so the GC does not care about these
    if(IS_OBJ(value)) markObject(AS_OBJ(value));
} 

static void markArray(ValueArray* array) {
    for(int i = 0; i < array->count; i++)
        markValue(array->values[i]);
} 

static void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*) object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif
    switch(object->type) {
        case OBJ_CLOSURE: {
            // closures reference the function it wraps and the array of pointers
            // to the upvalue it captures
            ObjClosure* closure = (ObjClosure*) object;
            markObject((Obj*) closure->function);
            for(int i = 0; i < closure->upvalueCount; i++)
                markObject((Obj*) closure->upvalues[i]);
            break;
        } 
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) object;
            markObject((Obj*) function->name); // must mark the function's name
            markArray(&function->chunk.constants); // as well as its constant array
            break;
        } 
        case OBJ_UPVALUE:
            // if an upvalue is closed the value lives in this field
            markValue(((ObjUpvalue*) object)->closed);
            break;
        // these shouldn't even be gray to begin with
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    } 
} 

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*) object, object->type);
#endif

	switch(object->type) {
        case OBJ_CLOSURE: {
            // we do *not* free the function because the closure does not own it
            // there could be several closures that reference the same function
            FREE(ObjClosure, object);
            break;
        } 
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*) object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            // there is no need here to free the function's name
            // because it is an ObjString.
            // so, the garbage collector can manage its lifetime for us.
            break;
        } 
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
		case OBJ_STRING: {
			ObjString* string = (ObjString*) object;
			// free whatever's been allocated
			FREE_ARRAY(char, string->chars, string->length + 1);
			// have to free the actual entire object
			FREE(ObjString, object);
			//reallocate(object, sizeof(ObjString) + sizeof(char)*(string->length + 1), 0);

			break;
		} 
        case OBJ_UPVALUE:
            // multiple upvalues can close over the same variable
            FREE(ObjUpvalue, object);
            break;
	} 
} 

static void markRoots() {
    // value stack
    for(Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    } 

    // global values
    markArray(&vm.globalValues);

    // CallFrame pointers to closures
    for(int i = 0; i < vm.frameCount; i++)
        markObject((Obj*) vm.frames[i].closure);

    // open upvalue list
    for(ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL;
            upvalue = upvalue->next) {
        markObject((Obj*) upvalue);
    } 

    // global names
    markTable(&vm.globalNames);
    // constant global names
    markTable(&constantGlobals);

    markCompilerRoots();
} 

static void traceReferences() {
    while(vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    } 
} 

// go through the objects linked list that we have
static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;

    while(object != NULL) {
        if(object->isMarked) {
            object->isMarked = false; // for the next marking
            previous = object; // previous is this object
            object = object->next; // iterate to next one
        } else {
            Obj* unreached = object;
            object = object->next; // iterate
            // remove `unreached` from the linked list
            if(previous != NULL) {
                previous->next = object; 
            } else {
                vm.objects = object; // or it's the start, so we reassign vm.objects
            } 

            freeObject(unreached);
        } 
    } 
} 

void collectGarbage() {
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
#endif

    markRoots(); // mark initial nodes
    traceReferences(); // graph traversal
    tableRemoveWhite(&vm.strings); // remove elements in string table that are white
    Obj* object = vm.objects;
/*
    while(object != NULL) {
        printf("'");
        printObject(OBJ_VAL(object));
        if(object->isMarked) {
            printf("' marked\n");
        } else {
            printf("' will be deleted\n");    
        } 
        object = object->next;
    } 
*/

    sweep(); // remove the white nodes

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
#endif
} 

void freeObjects() {
	Obj* object = vm.objects;
	while(object != NULL) {
		Obj* next = object-> next;
		freeObject(object);
		object = next;
	} 
    free(vm.grayStack);
} 
