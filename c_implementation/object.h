#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

// for pointers to a Value
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
	OBJ_STRING,
    OBJ_NATIVE,
    OBJ_FUNCTION
} ObjType;

// object "metadata" or "inheritor"
struct Obj {
	ObjType type;
	struct Obj* next;
}; 

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative; 

// string is an array of chars
struct ObjString {
	Obj obj;
	int length; // convenient for not walking the whole string
	uint32_t hash;
	char* chars;
};

ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length); 
void printObject(Value value);

// separate function because we need `value` twice,
// and if evaluating that produces side effects, we've got bad things goin on
static inline bool isObjType(Value value, ObjType type) {
	// short circuiting here
	return IS_OBJ(value) && AS_OBJ(value)->type == type;
} 

#endif
