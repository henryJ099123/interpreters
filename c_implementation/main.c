#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[]) {
	initVM();
	Chunk chunk;
	initChunk(&chunk);

	// remember, this returns an index to find the constant
	/*
	writeChunk(&chunk, OP_RETURN, 123);
	writeChunk(&chunk, OP_RETURN, 256);
	writeChunk(&chunk, OP_RETURN, 256);
	writeChunk(&chunk, OP_RETURN, 256);
	writeChunk(&chunk, OP_RETURN, 1);
	*/
	// to evaluate -((1.2 + 3.4) / 5.6):
	int constant = addConstant(&chunk, 1.2);
	writeChunk(&chunk, OP_CONSTANT, 123);
	writeChunk(&chunk, constant, 123);

	writeConstant(&chunk, 3.4, 123);
	writeChunk(&chunk, OP_ADD, 123);
	writeConstant(&chunk, 5.6, 123);
	writeChunk(&chunk, OP_DIVIDE, 123);

	writeChunk(&chunk, OP_NEGATE, 123);
	writeChunk(&chunk, OP_RETURN, 123);
	/*
	for(int i = 0; i < 300; i++)
		writeConstant(&chunk, (Value) i, i);
	*/
	disassembleChunk(&chunk, "test chunk");	
	interpret(&chunk);
	freeVM();
	freeChunk(&chunk);
	return 0;
} 
