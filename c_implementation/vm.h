#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "object.h"
#include "value.h"
#include "table.h"

// max callFrame depth
#define FRAMES_MAX 64
// redefine value stack's size in terms of callFrame depth for deep trees
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure* closure;
    // instead of storing return addresses the caller stores its own `ip`
    // when returning, the VM will jump to the ip of the caller's CallFrame
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;

	Value stack[STACK_MAX];
	Value* stackTop; // ptr to top of stack
	Table globalNames;
	ValueArray globalValues;
	Table strings;
    ObjUpvalue* openUpvalues;
	Obj* objects;
    // for the garbage collector
    int grayCount;
    int grayCapacity;
    Obj** grayStack;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

// expose the global variable
extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif
