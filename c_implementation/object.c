#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
	(type*) allocateObject(sizeof(type), objectType)

// recall that as Obj is the first field of all "Objs",
// we allocate space for the "outer" type and then
// assign this one's fields; the "outer" fields are assigned separately
static Obj* allocateObject(size_t size, ObjType type) {
	Obj* object = (Obj*)reallocate(NULL, 0, size);
	object->type = type;
	// insert at the head
	object->next = vm.objects;
	vm.objects = object;
	return object;
} 

// allocates new ObjString on the heap, like a constructor
// needs to allocate the Obj itself first though
static ObjString* allocateString(char* chars, int length) {
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	string->length = length;
	string->chars = chars;
	return string;
} 

// allows taking ownership of the chars immediately
// i.e. no need to copy characters and allocate new string array
// need to allocate new object, though
ObjString* takeString(char* chars, int length) {
	return allocateString(chars, length);
} 

ObjString* copyString(const char* chars, int length) {
	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	// have to terminate this ourselves
	// lexeme points at a range of characters
	heapChars[length] = '\0';
	return allocateString(heapChars, length);
} 

void printObject(Value value) {
	switch(OBJ_TYPE(value)) {
		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
	} 
} 

