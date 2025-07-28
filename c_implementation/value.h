#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

// enum for types of values the VM supports
typedef enum {
	VAL_BOOL,
	VAL_NIL,
	VAL_UNDEF,
	VAL_NUMBER,
	VAL_OBJ
} ValueType;

// field for type and field for union of all other values
// way to check types and also hold any kind of thing
typedef struct {
	ValueType type;
	union {
		bool boolean;
		double number;
		Obj* obj;
	} as;
} Value;

/***** check type *****/
// this is what that `type` field is for!
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_UNDEF(value) ((value).type == VAL_UNDEF)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

/***** LOX -> C *****/
// given a Value (of correct type), it's unwrapped to the C value
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

/***** C -> LOX *****/
// returns Value structs that properly take the correct thing
#define BOOL_VAL(value) 	((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL				((Value){VAL_NIL, {.number=0}})
#define UNDEF_VAL			((Value){VAL_UNDEF, {.number=0}})
#define NUMBER_VAL(value) 	((Value){VAL_NUMBER, {.number=value}})
#define OBJ_VAL(object)		((Value){VAL_OBJ, {.obj=(Obj*)object}})

typedef struct {
	int capacity;
	int count;
	Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void freeValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void printValue(Value value);

#endif
