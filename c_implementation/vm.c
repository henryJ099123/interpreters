#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"

// global variable?
VM vm;

static Value clockNative(int argCount, Value* args) {
    return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
} 

static void resetStack() {
	// move stack ptr all the way to the beginning
	vm.stackTop = vm.stack;
    vm.frameCount = 0;
} 

// declaring our own variadic function!
static void runtimeError(const char* format, ...) {
	// variadic function whatnot
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

    // essentially print out each function and line on the stack trace
    for(int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;

        // -1 here bc IP is already sitting on the next instruction to be executed,
        // but want to point backwards to failed instruction
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", getLine(&function->chunk, instruction));
        
        if(function->name == NULL) fprintf(stderr, "script\n");
        else fprintf(stderr, "%s()\n", function->name->chars);
    } 

	// print line information
	// subtract 1 because the failing instruction is one before the instruction ip points at
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
	size_t instruction = frame->ip - frame->function->chunk.code - 1;
	int line = getLine(&frame->function->chunk, instruction);
	fprintf(stderr, "[line %d] in script\n", line);
	resetStack();
} 

static void defineNative(const char* name, NativeFn function) {
    // pushes are for garbage collection
    // because `copyString` and `newNative` allocate memory
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    int newIndex = vm.globalValues.count;
    writeValueArray(&vm.globalValues, vm.stack[1]);
    tableSet(&vm.globalNames, AS_STRING(vm.stack[0]), NUMBER_VAL((double) newIndex));
    pop();
    pop();
} 

void initVM() {
	resetStack();
	vm.objects = NULL;
//	initTable(&vm.globals);
	initTable(&vm.globalNames);
	initValueArray(&vm.globalValues);

	initTable(&vm.strings);

    defineNative("clock", clockNative);
} 

void freeVM() {
//	freeTable(&vm.globals);
	freeTable(&vm.globalNames);
	freeValueArray(&vm.globalValues);
	freeTable(&vm.strings);
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

static bool call(ObjFunction* function, int argCount) {
    if(argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
        return false;
    } 

    if(vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    } 

    // new stack frame
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    // the -1 is to account for stack slot 0, which is set aside for methods
    frame->slots = vm.stackTop - argCount - 1;
    return true;
} 

// need to check if a function or something actually callable is being called
static bool callValue(Value callee, int argCount) {
    if(IS_OBJ(callee)) {
        switch(OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            // call the native function
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            } 
            default:
                break; // non-callable
        } 
    } 
    runtimeError("Calling a non-callable thing.");
    return false;
} 

// only things that are false are nil and the actual value false
static bool isFalsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
} 

static void concatenate() {
	ObjString* b = AS_STRING(pop());
	ObjString* a = AS_STRING(pop());

	// this works if the string is always new...
	// but no good if it already does.
	/*
	ObjString* result = makeString(a->length + b->length);
	memcpy(result->chars, a->chars, a->length);
	memcpy(result->chars + a->length, b->chars, b->length);
	result->chars[result->length] = '\0';
	result->hash = hashString(result->chars, result->length);
	*/

	// dynamically make new string
	int length = a->length + b->length;
	char* chars = ALLOCATE(char, length+1);
	memcpy(chars, a->chars, a->length);
	memcpy(chars + a->length, b->chars, b->length);
	chars[length] = '\0';

	/*
	// have to use copyString, because of the ways strings are stored
	ObjString* result = copyString(chars, length);
	*/

	// use `takeString` here because the chars are already dynamically allocated
	// no need to copy them
	ObjString* result = takeString(chars, length);
	push(OBJ_VAL(result));
} 

// the heart and soul of the virtual machine
static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++) // advance!
#define READ_BYTE_LONG() (0x00FFFFFF & \
		((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE())))
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (frame->function->chunk.constants.values[ READ_BYTE_LONG() ] )
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_LONG_STRING() AS_STRING(READ_LONG_CONSTANT())
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
		printf("         ");
		for(Value* slot = vm.stack; slot < vm.stackTop; slot++) {
			printf("[ ");
			printValue(*slot);
			printf(" ]");
		} 
		printf("\n");
		// recall that this instruction takes an offset --> ptr math
		disassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
		/*
		printf("~~hash table of strs~~\n");
		for(int i = 0; i < vm.strings.capacity; i++) {
			if(vm.strings.entries[i].key != NULL)
				printf("%d %d %s\n", i, vm.strings.entries[i].key->hash, vm.strings.entries[i].key->chars);
		} 
		printf("~~end~~\n");
		*/
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
				push(constant);
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
			// has already executed code for expression and leaves it on the stack
			// note: no pushing!
			case OP_PRINT: {
				printValue(pop());
				printf("\n");
				break;
			} 
			case OP_POP: pop(); break;
            case OP_DUP: push(peek(0)); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            } 
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            } 
			case OP_DEFINE_GLOBAL: {
				// take index off of chunk
				// this represents some global variable
				vm.globalValues.values[READ_BYTE()] = peek(0);
				pop();
				/*
				ObjString* name = READ_STRING();
				tableSet(&vm.globals, name, peek(0));
				pop();
				*/
				break;
			} 
			case OP_DEFINE_GLOBAL_LONG: {
				vm.globalValues.values[READ_BYTE_LONG()] = peek(0);
				pop();
			/*
				ObjString* name = READ_LONG_STRING();
				tableSet(&vm.globals, name, peek(0));
				pop();
			*/
				break;
			} 
			case OP_GET_GLOBAL: {
				uint8_t index = READ_BYTE();
				if(IS_UNDEF(vm.globalValues.values[index])) {
					ObjString* key = tableFindKey(&vm.globalNames, NUMBER_VAL((double) index));
					if(key == NULL)	runtimeError("Undefined variable.");
					else runtimeError("Undefined variable '%s'.", key->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
				push(vm.globalValues.values[index]);
			/*
				ObjString* name = READ_STRING();
				Value value;
				if(!tableGet(&vm.globals, name, &value)) {
					runtimeError("Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
				push(value);
			*/
				break;
			} 
			case OP_GET_GLOBAL_LONG: {
				uint32_t index = READ_BYTE_LONG();
				if(IS_UNDEF(vm.globalValues.values[index])) {
					ObjString* key = tableFindKey(&vm.globalNames, NUMBER_VAL((double) index));
					if(key == NULL)	runtimeError("Undefined variable.");
					else runtimeError("Undefined variable '%s'.", key->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
				push(vm.globalValues.values[index]);
			/*
				ObjString* name = READ_LONG_STRING();
				Value value;
				if(!tableGet(&vm.globals, name, &value)) {
					runtimeError("Undefined variable '%s'.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
				push(value);
			*/
				break;
			} 
			case OP_SET_GLOBAL: {
				uint8_t index = READ_BYTE();	
				if(IS_UNDEF(vm.globalValues.values[index])) {
					ObjString* key = tableFindKey(&vm.globalNames, NUMBER_VAL((double) index));
					if(key == NULL)	runtimeError("Trying to set undefined variable.");
					else runtimeError("Trying to set undefined variable '%s'.", key->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
				vm.globalValues.values[index] = peek(0);
			/*
				ObjString* name = READ_STRING();
				// tableSet returns `true` if it is NEW thing being added
				// i.e. runtime error if variable hasn't been declared
				if(tableSet(&vm.globals, name, peek(0))) {
					// need to delete the zombie
					tableDelete(&vm.globals, name);
					runtimeError("Undefined variable '%s' being assigned.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
			*/
				break;
			} 
			case OP_SET_GLOBAL_LONG: {
				uint8_t index = READ_BYTE_LONG();	
				if(IS_UNDEF(vm.globalValues.values[index])) {
					ObjString* key = tableFindKey(&vm.globalNames, NUMBER_VAL((double) index));
					if(key == NULL)	runtimeError("Trying to set undefined variable.");
					else runtimeError("Trying to set undefined variable '%s'.", key->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
				vm.globalValues.values[index] = peek(0);
				/*
				ObjString* name = READ_LONG_STRING();
				// tableSet returns `true` if it is NEW thing being added
				// i.e. runtime error if variable hasn't been declared
				if(tableSet(&vm.globals, name, peek(0))) {
					// need to delete the zombie
					tableDelete(&vm.globals, name);
					runtimeError("Undefined variable '%s' being assigned.", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				} 
				*/
				break;
			} 
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            } 
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if(isFalsey(peek(0))) frame->ip += offset;
                break;
            } 
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            } 
            case OP_CALL: {
                // get number of arguments as a parameter
                int argCount = READ_BYTE();
                // can get the function on the stack by counting backwards from that
                if(!callValue(peek(argCount), argCount))
                    return INTERPRET_RUNTIME_ERROR;
                // puts a new CallFrame on the stack
                frame = &vm.frames[vm.frameCount - 1];
                break;
            } 
			case OP_RETURN: {
                // pop return value and discard called function's stack window
                Value result = pop();
                vm.frameCount--;

                // exit interpreter
                if(vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                } 

                // put top of the stack back past the slots used for the function
                vm.stackTop = frame->slots;
                // put the return value at a lower location
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
			} 
		} 
	} 
#undef READ_BYTE
#undef READ_BYTE_LONG
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_LONG_CONSTANT
#undef READ_STRING
#undef READ_LONG_STRING
#undef BINARY_OP
} 

InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);
    if(function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function)); // reserved for VM

    call(function, 0);

	//execute the vm
	return run();
} 
