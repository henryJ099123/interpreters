#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

// a "constructor" for a chunk
void initChunk(Chunk* chunk) {
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	chunk->lines = NULL;
	initValueArray(&chunk->constants);
} 

void freeChunk(Chunk* chunk) {
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	FREE_ARRAY(int, chunk->lines, chunk->capacity);
	freeValueArray(&chunk->constants);

	// doesn't actually *delete* the chunk but reassigns its values
	// just in case for some bad apples...zeroes out the info
	initChunk(chunk);
} 

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
	// reallocate array as needed
	if(chunk->capacity < chunk->count + 1) {
		int oldCapacity = chunk->capacity;
		chunk->capacity = GROW_CAPACITY(oldCapacity);
		chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
		chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
	} 

	// append the byte
	chunk->code[chunk->count] = byte;
	chunk->lines[chunk->count] = line;
	chunk->count++;
} 

int addConstant(Chunk* chunk, Value value) {
	writeValueArray(&chunk->constants, value);

	// return index where the constant was appended
	return chunk->constants.count - 1;
} 
