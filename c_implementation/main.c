#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include <time.h>
#include <stdio.h>

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
	for(int i = 0; i < 100000 ; i++)
		writeChunk(&chunk, OP_NEGATE, 123);

	clock_t start = clock();
//	disassembleChunk(&chunk, "test chunk");	
	interpret(&chunk);
	freeVM();
	freeChunk(&chunk);
	clock_t end = clock();
	printf("s %g\n", (float)(end-start)/CLOCKS_PER_SEC);
	return 0;
} 
