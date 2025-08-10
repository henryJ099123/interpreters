#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "object.h"
#include "memory.h"

// global variable?
VM vm;

/* ----- NATIVE FUNCTIONS ----- */
static char* arityCheck(int trueArity, int numArguments, const char* name) {
    if(trueArity == numArguments) return NULL;
    char* errorMsg = ALLOCATE(char, 61 + strlen(name)); 
    snprintf(errorMsg, 61 + strlen(name), 
        "Expected %d arguments but got %d in native function '%s'.",
        trueArity, numArguments, name);
    return errorMsg;
} 


static Value clockNative(int argCount, Value* args, bool* wasError) {
    char* arityError = arityCheck(0, argCount, "clock");
    if(arityError != NULL) {
        *wasError = true;
        return OBJ_VAL(takeString(arityError, (int) strlen(arityError)));
    } 
    return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
} 

static Value userInputNative(int argCount, Value* args, bool* wasError) {
    char* arityError = arityCheck(0, argCount, "inputLine");
    if(arityError != NULL) {
        *wasError = true;
        return OBJ_VAL(takeString(arityError, (int) strlen(arityError)));
    } 
    char buffer[BUFSIZ];
    fgets(buffer, BUFSIZ, stdin);
    if(buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';
    return OBJ_VAL(copyString(buffer, (int) strlen(buffer)));
} 

static Value sqrtNative(int argCount, Value* args, bool* wasError) {
    char* arityError = arityCheck(1, argCount, "sqrt");
    if(arityError != NULL) {
        *wasError = true;
        return OBJ_VAL(takeString(arityError, (int) strlen(arityError)));
    } 
    if(!IS_NUMBER(args[0])) {
        char errStr[] = "'sqrt' expects a number as argument.";
        *wasError = true;
        return OBJ_VAL(copyString(errStr, (int) strlen(errStr))); 
    }
    double num = AS_NUMBER(args[0]);
    if(num < 0) {
        char errStr[] = "'sqrt' must take a nonnegative number."; 
        *wasError = true;
        return OBJ_VAL(copyString(errStr, (int) strlen(errStr)));
    } 

    return NUMBER_VAL(sqrt(num));
} 

static void resetStack() {
	// move stack ptr all the way to the beginning
	vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = NULL;
} 

// declaring our own variadic function!
static void runtimeError(const char* format, ...) {
	// variadic function whatnot
    // prints the actual error message
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

    // print the stack trace
    for(int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;

        // -1 here bc IP is already sitting on the next instruction to be executed,
        // but want to point backwards to failed instruction
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", getLine(&function->chunk, instruction));
        
        if(function->name == NULL) fprintf(stderr, "script\n");
        else fprintf(stderr, "%s()\n", function->name->chars);
    } 

	resetStack();
} 

static void defineNative(const char* name, NativeFn function) {
    // pushes are for garbage collection
    // because `copyString` and `newNative` allocate memory
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    int newIndex = vm.globalValues.count;
    tableSet(&vm.globalNames, AS_STRING(vm.stack[0]), NUMBER_VAL((double) newIndex));
    writeValueArray(&vm.globalValues, vm.stack[1]);
    pop();
    pop();
} 

void initVM() {
	resetStack();
	vm.objects = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024; // magic number

    // for GC
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = NULL;

    // globals and constant globals
	initTable(&vm.globalNames);
	initValueArray(&vm.globalValues);

    // strings
	initTable(&vm.strings);

    // native functions
    defineNative("clock", clockNative);
    defineNative("sqrt", sqrtNative);
    defineNative("inputLine", userInputNative);
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

static bool call(ObjClosure* closure, int argCount) {
    if(argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d in function '%s'.", 
                     closure->function->arity, argCount, closure->function->name->chars);
        return false;
    } 

    if(vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    } 

    // new stack frame
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    // the -1 is to account for stack slot 0, which is set aside for methods
    frame->slots = vm.stackTop - argCount - 1;
    return true;
} 

// need to check if a function or something actually callable is being called
static bool callValue(Value callee, int argCount) {
    if(IS_OBJ(callee)) {
        switch(OBJ_TYPE(callee)) {
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                // constructor call
                // bypass the arguments? No popping?
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
                return true;
            } 
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCount);
            // call the native function
            case OBJ_NATIVE: {
                ObjNative* native = AS_NATIVE(callee);
                bool wasError = false;
                Value result = native->function(argCount, vm.stackTop - argCount, &wasError);
                vm.stackTop -= argCount + 1;
                if(wasError) {
                    runtimeError("%s", AS_CSTRING(result));
                    return false;
                } 
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

static ObjUpvalue* captureUpvalue(Value* local) {
    ObjUpvalue* prevUpvalue = NULL;

    // start at head of list and traverse down
    ObjUpvalue* upvalue = vm.openUpvalues;
    while(upvalue != NULL && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    } 

    // if we've landed on the actual upvalue
    if(upvalue != NULL && upvalue->location == local)
        return upvalue;

    ObjUpvalue* createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    // insert at head or after the previous
    if(prevUpvalue == NULL)
        vm.openUpvalues = createdUpvalue;
    else
        prevUpvalue->next = createdUpvalue;

    return createdUpvalue;
} 

// closes all upvalues up to a certain point, `last`
static void closeUpvalues(Value* last) {
    while(vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
        // top of list
        ObjUpvalue* upvalue = vm.openUpvalues;
        // store it's value as the value of the variable
        // this is the heap location
        upvalue->closed = *upvalue->location;
        // store it's location as that value's address
        // instead of pointing to the stack, it now points to its other field
        upvalue->location = &upvalue->closed;
        // get rid of it
        vm.openUpvalues = upvalue->next;
    } 
} 
// only things that are false are nil and the actual value false
static bool isFalsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
} 

static void concatenate() {
	ObjString* b = AS_STRING(peek(0));
	ObjString* a = AS_STRING(peek(1));

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
    pop();
    pop();
	push(OBJ_VAL(result));
} 

// the heart and soul of the virtual machine
static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++) // advance!
#define READ_BYTE_LONG() (0x00FFFFFF & \
		((READ_BYTE() << 16) | (READ_BYTE() << 8) | (READ_BYTE())))
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (frame->closure->function->chunk.constants.values[ READ_BYTE_LONG() ] )
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
		disassembleInstruction(&frame->closure->function->chunk, 
                (int)(frame->ip - frame->closure->function->chunk.code));
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
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                // we look up the reference and dereference it
                push(*frame->closure->upvalues[slot]->location);
                break;
            } 
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
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
            case OP_GET_PROPERTY: {
                if(!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                } 

                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_STRING();
                Value value;

                if(tableGet(&instance->fields, name, &value)) {
                    pop();
                    push(value);
                    break;
                } 

                runtimeError("Undefine property '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            } 
            case OP_GET_PROPERTY_LONG: {
                if(!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                } 

                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjString* name = READ_LONG_STRING();
                Value value;

                if(tableGet(&instance->fields, name, &value)) {
                    pop();
                    push(value);
                    break;
                } 

                runtimeError("Undefine property '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            } 
            case OP_SET_PROPERTY: {
                if(!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields to set.");
                    return INTERPRET_RUNTIME_ERROR;
                } 
                // stack is currently [ instance ][ Value ]
                ObjInstance* instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);
                break;
            } 
            case OP_SET_PROPERTY_LONG: {
                if(!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields to set.");
                    return INTERPRET_RUNTIME_ERROR;
                } 
                // stack is currently [ instance ][ Value ]
                ObjInstance* instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_LONG_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);
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
                // puts a new CallFrame on the stack for the called function
                // or just reaccesses the same one, for a native function
                frame = &vm.frames[vm.frameCount - 1];
                break;
            } 
            case OP_CLOSURE: {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));

                for(int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if(isLocal)
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    // as OP_CLOSURE is emitted at the end of a function declaration,
                    // the *current* function is the surrounding one...
                    // thus, the current CallFrame has the upvalue
                    else
                        closure->upvalues[i] = frame->closure->upvalues[index];
                } 
                break;
            } 
            case OP_CLOSURE_LONG: {
                ObjFunction* function = AS_FUNCTION(READ_LONG_CONSTANT());
                ObjClosure* closure = newClosure(function);
                push(OBJ_VAL(closure));

                for(int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if(isLocal)
                        closure->upvalues[i] = captureUpvalue(frame->slots + index);
                    // as OP_CLOSURE is emitted at the end of a function declaration,
                    // the *current* function is the surrounding one...
                    // thus, the current CallFrame has the upvalue
                    else
                        closure->upvalues[i] = frame->closure->upvalues[index];
                } 
                break;
            } 
            case OP_CLOSE_UPVALUE:
                closeUpvalues(vm.stackTop - 1);
                pop(); // still need to pop the local
                break;
			case OP_RETURN: {
                // pop return value and discard called function's stack window
                Value result = pop();
                // close all remaining open upvalues owned by the returning function
                closeUpvalues(frame->slots);
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
            case OP_CLASS:
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
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

    // this weird push-pop-push behavior is for the garbage collector
    push(OBJ_VAL(function)); // reserved for VM
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

	//execute the vm
	return run();
} 
