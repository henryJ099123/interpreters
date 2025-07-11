#include <stdio.h>
#include "common.h"
#include "vm.h"
#include "debug.h"

// global variable?
VM vm;

void initVM() {

} 

void freeVM() {

} 

// the heart and soul of the virtual machine
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++) // advance!
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[ 0x00FFFFFF & \
		((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE()))])

	for(;;) {
#ifdef DEBUG_TRACE_EXECUTION
		// recall that this instruction takes an offset --> ptr math
		disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
		uint8_t instruction;
		switch(instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				printValue(constant);
				printf("\n");
				break;
		    }
			case OP_CONSTANT_LONG: {
				Value constant = READ_LONG_CONSTANT();
				printValue(constant);
				printf("\n");
				break;
			} 
			case OP_RETURN: {
				return INTERPRET_OK;
			} 
		} 
	} 
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_LONG_CONSTANT
} 

InterpretResult interpret(Chunk* chunk) {
	vm.chunk = chunk;
	vm.ip = vm.chunk->code;
	return run(); // actually runs the bytecode
} 
