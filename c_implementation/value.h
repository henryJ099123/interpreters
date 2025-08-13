#ifndef clox_value_h
#define clox_value_h

#include <string.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

// everything zero except the quiet NaN bits
#define SIGN_BIT ((uint64_t) 0x8000000000000000)
#define QNAN ((uint64_t)    0x7ffc000000000000)

#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3
#define TAG_UNDEF 0 // is 0 being reserved...?

typedef uint64_t Value;

// the number is a number if we've set all of the quiet NaN bits
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)

// any negative number ALSO has the sign bit checked,
// so we have to check that all the exponent bits are 1 too
#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_UNDEF(value) ((value) == UNDEF_VAL)

#define AS_NUMBER(value) valueToNum(value)
#define AS_BOOL(value) ((value) == TRUE_VAL)

// clears the top 13 or so bits for the pointer
#define AS_OBJ(value) \
    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

// ptr itself is 64 bits in principle,
// but on most architectures everything above the 48th bit is 0
// casting here to satisfy C compilers
#define OBJ_VAL(obj) \
    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

#define NUMBER_VAL(num) numToValue(num)
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
// casting is for the compiler. these is a singleton
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define UNDEF_VAL ((Value)(uint64_t)(QNAN | TAG_UNDEF))

// compiler should eliminate all of this memcpy whatnot
static inline Value numToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
} 

static inline double valueToNum(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
} 

/* use the following if the memcpy is not optimized away:
 * double valueToNum(Value value) {
 *     union {
 *         uint64_t bits;
 *         double num;
 *     } data;
 *     
 *     data.bits = value;
 *     return data.num;
 * } 
*/

#else 

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

#endif

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
