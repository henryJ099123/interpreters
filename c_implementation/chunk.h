#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// each instruction has 1-byte opcode
typedef enum {
	OP_CONSTANT_LONG,
	OP_CONSTANT,
	OP_RETURN,
} OpCode;

// bytecode struct to hold instruction and other data
typedef struct {
	int count;
	int capacity;
	uint8_t* code;
	int lcount;
	int lcapacity;
	int* lines;
	ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
void writeConstant(Chunk* chunk, Value value, int line);
int getLine(Chunk* chunk, int index);

#endif


