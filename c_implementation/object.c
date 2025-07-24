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
	// insert at the head
	object->next = vm.objects;
	vm.objects = object;
	// just check if it is there
	return object;
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
	tableSet(&vm.strings, string, NIL_VAL);
	return string;
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

// the actual hash function
uint32_t hashString(const char* key, int length) {
	uint32_t hash = 2166136261u;
	for(int i = 0; i < length; i++) {
		hash ^= (uint8_t) key[i];
		hash *= 16777619;
	} 
	return hash;
} 

void printObject(Value value) {
	switch(OBJ_TYPE(value)) {
		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
	} 
} 

