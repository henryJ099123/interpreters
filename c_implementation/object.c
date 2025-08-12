#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

#define ALLOCATE_OBJ(type, objectType) \
	(type*) allocateObject(sizeof(type), objectType)

#define ALLOCATE_STRING(length) \
	(ObjString*) allocateObject(sizeof(ObjString) + sizeof(char)*(length), OBJ_STRING)

// recall that as Obj is the first field of all "Objs",
// we allocate space for the "outer" type and then
// assign this one's fields; the "outer" fields are assigned separately
static Obj* allocateObject(size_t size, ObjType type) {
	Obj* object = (Obj*)reallocate(NULL, 0, size);
	object->type = type;
    object->isMarked = false;
	// insert at the head
	object->next = vm.objects;
	vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*) object, size, type);
#endif

	return object;
} 

ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
} 

ObjInstance* newInstance(ObjClass* klass) {
    ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    initTable(&instance->fields);
    return instance;
} 

ObjClass* newClass(ObjString* name) {
    ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    initTable(&klass->methods);
    return klass;
} 

ObjClosure* newClosure(ObjFunction* function) {
    // we know exactly how big the array needs to be
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
    // this is important for the garbage collector (it shouldn't see uninitialized memory)
    for(int i = 0; i < function->upvalueCount; i++)
        upvalues[i] = NULL;

    ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    // closures own the array containing pointers to the upvalues
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
} 

// function in a blank state
ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
} 

ObjNative* newNative(NativeFn function) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
} 

/*
ObjString* makeString(int length) {
	ObjString* string = ALLOCATE_STRING(length + 1);
	string->length = length;
	// we don't need values, only care about the actual string being in there
	tableSet(&vm.strings, string, NIL_VAL);
	return string;
}
*/

// allocates new ObjString on the heap, like a constructor
// needs to allocate the Obj itself first though
static ObjString* allocateString(char* chars, int length, uint32_t hash) {
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	string->hash = hash;
    push(OBJ_VAL(string));
	tableSet(&vm.strings, string, NIL_VAL);
    pop();
	return string;
}

// the actual hash function
static uint32_t hashString(const char* key, int length) {
	uint32_t hash = 2166136261u;
	for(int i = 0; i < length; i++) {
		hash ^= (uint8_t) key[i];
		hash *= 16777619;
	} 
	return hash;
} 

// allows taking ownership of the chars immediately
// i.e. no need to copy characters and allocate new string array
// need to allocate new object, though
// also need to free the memory string passed in and don't need the duplicate
ObjString* takeString(char* chars, int length) {
	uint32_t hash = hashString(chars, length);
	ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
	if(interned != NULL) {
		FREE_ARRAY(char, chars, length + 1);
		return interned;
	} 
	return allocateString(chars, length, hash);
} 

// no string ownership anymore, as everything is stored in the same struct
ObjString* copyString(const char* chars, int length) {
	uint32_t hash = hashString(chars, length);
	// if the string already is interned we just return that one
	ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
	if(interned != NULL) return interned;
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	// have to terminate this ourselves
	// lexeme points at a range of characters
	heapChars[length] = '\0';
	return allocateString(heapChars, length, hash);

/*
	ObjString* string = makeString(length);
	memcpy(string->chars, chars, length);
	string->chars[length] = '\0';
	string->hash = hash;
	return string;
*/
} 

ObjUpvalue* newUpvalue(Value* slot) {
    ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
} 


static void printFunction(ObjFunction* function) {
    if(function->name == NULL) {
        printf("<script>");
        return;
    } 
    printf("<fn %s>", function->name->chars);
} 

void printObject(Value value) {
	switch(OBJ_TYPE(value)) {
        // these just look like functions to the user
        case OBJ_BOUND_METHOD:
            printFunction(AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_INSTANCE:
            printf("%s instance", AS_INSTANCE(value)->klass->name->chars);
            break;
        case OBJ_CLASS:
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
        case OBJ_UPVALUE:
            // shouldn't really be accessible by the user
            printf("upvalue");
            break;
	} 
} 

