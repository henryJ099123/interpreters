#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"

// global variable?
VM vm;

static void resetStack() {
	// move stack ptr all the way to the beginning
	vm.stackTop = vm.stack;
} 

// declaring our own variadic function!
static void runtimeError(const char* format, ...) {
	// variadic function whatnot
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	// print line information
	// subtract 1 because the failing instruction is one before the instruction ip points at
	size_t instruction = vm.ip - vm.chunk->code - 1;
	int line = getLine(vm.chunk, instruction);
	fprintf(stderr, "[line %d] in script\n", line);
	resetStack();
} 

void initVM() {
	resetStack();
	vm.objects = NULL;
} 

void freeVM() {
	freeObjects();
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

// look as far into the stack as desired
// need to subtract by 1 because this is the top of the stack
static Value peek(int distance) {
	return vm.stackTop[-1 - distance];
} 

// only things that are false are nil and the actual value false
static bool isFalsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
} 

static void concatenate() {
	ObjString* b = AS_STRING(pop());
	ObjString* a = AS_STRING(pop());

	// dynamically make new string
	int length = a->length + b->length;
	char* chars = ALLOCATE(char, length+1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	// use `takeString` here because the chars are already dynamically allocated
	// no need to copy them
	ObjString* result = takeString(chars, length);
	push(OBJ_VAL(result));
} 

// the heart and soul of the virtual machine
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++) // advance!
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[ 0x00FFFFFF & \
		((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE()))])
// need the `do`...`while` to force adding a semicolon at the end
// this is...quite the macro. notice the wrapper to use is passed as a macro param
#define BINARY_OP(valueType, op) \
	do { \
		if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
			runtimeError("Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		double b = AS_NUMBER(pop()); \
		double a = AS_NUMBER(pop()); \
		push(valueType(a op b)); \
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
			case OP_NIL: push(NIL_VAL); break;
			case OP_TRUE: push(BOOL_VAL(true)); break;
			case OP_FALSE: push(BOOL_VAL(false)); break;
			case OP_EQUAL: {
				Value b = pop();
				Value a = pop();
				push(BOOL_VAL(valuesEqual(a, b)));
				break;
		    } 
			case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
			case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
			case OP_NOT:
				push(BOOL_VAL(isFalsey(pop()))); break;
			// firsts pops off the value; then negates; then pops
			case OP_NEGATE: 
				if(!IS_NUMBER(peek(0))) {
					runtimeError("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				} 
				push(NUMBER_VAL(-AS_NUMBER(pop()))); break;
				// vm.stackTop[-1] = NUMBER_VAL(-AS_NUMBER(vm.stackTop[-1])); break;
			case OP_ADD: {
				if(IS_STRING(peek(0)) && IS_STRING(peek(1)))
					concatenate();
				else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
					double b = AS_NUMBER(pop());
					double a = AS_NUMBER(pop());
					push(NUMBER_VAL(a + b));
				} else {
					runtimeError("Operands must be numbers or strings.");
					return INTERPRET_RUNTIME_ERROR;
				} 
				break;
			}
			case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
			case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
			case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
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
	Chunk chunk;
	initChunk(&chunk);
	// compile's actually giving something important now
	if(!compile(source, &chunk)) {
		freeChunk(&chunk);
		return INTERPRET_COMPILE_ERROR;
	} 

	//init the vm
	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	//execute the vm
	InterpretResult result = run();
	freeChunk(&chunk);
	return result;
} 
