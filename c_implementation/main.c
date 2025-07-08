#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[]) {
	Chunk chunk;
	initChunk(&chunk);

	// remember, this returns an index to find the constant
	int constant = addConstant(&chunk, 1.2);
	writeChunk(&chunk, OP_CONSTANT, 123);
	writeChunk(&chunk, constant, 123);
	writeChunk(&chunk, OP_RETURN, 123);
	writeChunk(&chunk, OP_RETURN, 256);
	writeChunk(&chunk, OP_RETURN, 256);
	writeChunk(&chunk, OP_RETURN, 256);
	writeChunk(&chunk, OP_RETURN, 1);
	
	disassembleChunk(&chunk, "test chunk");
	freeChunk(&chunk);
	return 0;
} 
