#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// each instruction has 1-byte opcode
typedef enum {
	OP_CONSTANT_LONG,
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_NEGATE,
	OP_PRINT,
	OP_POP,
	OP_DEFINE_GLOBAL,
	OP_DEFINE_GLOBAL_LONG,
	OP_GET_GLOBAL,
	OP_GET_GLOBAL_LONG,
	OP_SET_GLOBAL,
	OP_SET_GLOBAL_LONG,
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
void writeLongConstant(Chunk* chunk, int constant, int line);
int getLine(Chunk* chunk, int index);

#endif


