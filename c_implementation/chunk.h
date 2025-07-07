#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

// each instruction has 1-byte opcode
typedef enum {
	OP_RETURN,
} OpCode;

// bytecode struct to hold instruction and other data
typedef struct {
	int count;
	int capacity;
	uint8_t* code;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif


