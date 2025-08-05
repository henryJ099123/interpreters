#include <stdlib.h>

#include "memory.h"
#include "vm.h"

// seems just to be a wrapper on `realloc`
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
	if(newSize == 0) {
		free(pointer);
		// return NULL ptr? yikes
		return NULL;
	} 

	void* result = realloc(pointer, newSize);
	if(result == NULL) exit(1);
	return result;
} 

static void freeObject(Obj* object) {
	switch(object->type) {
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
	} 
} 

void freeObjects() {
	Obj* object = vm.objects;
	while(object != NULL) {
		Obj* next = object-> next;
		freeObject(object);
		object = next;
	} 
} 
