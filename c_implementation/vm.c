#include <stdio.h>
#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"

// global variable?
VM vm;

static void resetStack() {
	// move stack ptr all the way to the beginning
	vm.stackTop = vm.stack;
} 

void initVM() {
	resetStack();
} 

void freeVM() {

} 

void push(Value value) {
	// dereference the stack pointer and assign it as so
	*vm.stackTop = value;
	// increment the stack pointer
	vm.stackTop++;
} 

Value pop() {
	// move down the stack pointer
	// marks that cell as no longer in use
	vm.stackTop--;
	// return what's there
	return *vm.stackTop;
} 

// the heart and soul of the virtual machine
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++) // advance!
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[ 0x00FFFFFF & \
		((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE()))])
// need the `do`...`while` to force adding a semicolon at the end
#define BINARY_OP(op) \
	do { \
		double b = pop(); \
		double a = pop(); \
		push(a op b); \
	} while(false)

	for(;;) {
#ifdef DEBUG_TRACE_EXECUTION
		printf("        ");
		for(Value* slot = vm.stack; slot < vm.stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		} 
		printf("\n");
		// recall that this instruction takes an offset --> ptr math
		disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
		uint8_t instruction;
		switch(instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				push(constant);
				break;
		    }
			case OP_CONSTANT_LONG: {
				Value constant = READ_LONG_CONSTANT();
				printValue(constant);
				printf("\n");
				break;
			} 
			// firsts pops off the value; then negates; then pops
			case OP_NEGATE: 
				vm.stackTop[-1] *= -1; break;
			case OP_ADD: BINARY_OP(+); break;
			case OP_SUBTRACT: BINARY_OP(-); break;
			case OP_MULTIPLY: BINARY_OP(*); break;
			case OP_DIVIDE: BINARY_OP(/); break;
			case OP_RETURN: {
				// for now, just print the last thing on the stack
				printValue(pop());
				printf("\n");
				return INTERPRET_OK;
			} 
		} 
	} 
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_LONG_CONSTANT
#undef BINARY_OP
} 

InterpretResult interpret(const char* source) {
	compile(source);
	return INTERPRET_OK;
} 
