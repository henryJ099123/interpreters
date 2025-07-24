#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

// for pointers to a Value
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
	OBJ_STRING,
} ObjType;

struct Obj {
	ObjType type;
	struct Obj* next;
}; 

// string is an array of chars
struct ObjString {
	Obj obj;
	int length; // convenient for not walking the whole string
	uint32_t hash;
	char* chars;
};


ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length); 
uint32_t hashString(const char* key, int length);
void printObject(Value value);

// separate function because we need `value` twice,
// and if evaluating that produces side effects, we've got bad things goin on
static inline bool isObjType(Value value, ObjType type) {
	// short circuiting here
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
} 

#endif
