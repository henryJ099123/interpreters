#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

// fixed size stack, for now
#define STACK_MAX 256

typedef struct {
	Chunk* chunk;
	uint8_t* ip; // faster to deref a ptr
	Value stack[STACK_MAX];
	Value* stackTop; // ptr to top of stack
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif
