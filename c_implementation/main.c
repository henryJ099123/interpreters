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
	int constant = addConstant(&chunk, 1.2);
	writeChunk(&chunk, OP_CONSTANT, 123);
	writeChunk(&chunk, constant, 123);
	for(int i = 0; i < 300; i++)
		writeConstant(&chunk, (Value) i, i);
	writeChunk(&chunk, OP_RETURN, 123);
	
	interpret(&chunk);
	freeVM();
	freeChunk(&chunk);
	return 0;
} 
