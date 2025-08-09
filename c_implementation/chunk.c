#include <stdlib.h>
#include "chunk.h"
#include "memory.h"
#include "vm.h"

// a "constructor" for a chunk
void initChunk(Chunk* chunk) {
	chunk->count = 0;
	chunk->capacity = 0;
	chunk->code = NULL;
	chunk->lcount = 0;
	chunk->lcapacity = 0;
	chunk->lines = NULL;
	initValueArray(&chunk->constants);
} 

void freeChunk(Chunk* chunk) {
	FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
	FREE_ARRAY(int, chunk->lines, chunk->lcapacity);
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
	}

	chunk->code[chunk->count] = byte;
	chunk->count++;

	// handling line counts
	// organization of the line array:
	// line_no count

	// if lines are the same just keep going
	if(chunk->lcount > 0 && chunk->lines[chunk->lcount - 2] == line) {
		return;
	} 
	
	// regrow if needed
	if(chunk->lcapacity < chunk->lcount + 2) {
		int oldCapacity = chunk->lcapacity;
		chunk->lcapacity = GROW_CAPACITY(oldCapacity);
		chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->lcapacity);
	} 
	
	// append a new element
	// set the next element to the line
	// set the element after that to be the new index element here
	chunk->lines[chunk->lcount] = line;
	chunk->lcount += 2;
	chunk->lines[chunk->lcount - 1] = chunk->count - 1; // set offset herey
} 

// this writes the actual index onto the chunk array
// this is used several times in the compiler.c file
void writeLongConstant(Chunk* chunk, int constant, int line) {
	writeChunk(chunk, (uint8_t) constant >> 16, line);
	writeChunk(chunk, (uint8_t) (constant >> 8) & 0xFF, line);
	writeChunk(chunk, (uint8_t) constant & 0xFF, line);
} 

// this takes care of writing a constant of any size
void writeConstant(Chunk* chunk, Value value, int line) {
	int constant = addConstant(chunk, value);
	if(chunk->constants.count <= UINT8_MAX) {
		writeChunk(chunk, OP_CONSTANT, line);
		writeChunk(chunk, constant, line);
		return;
	} 
	// will store in big-endian fashion
	writeChunk(chunk, OP_CONSTANT_LONG, line);
	writeLongConstant(chunk, constant, line);
} 

int getLine(Chunk* chunk, int index) {

	int i = 0;
	// must force some subtraction
	while(index - chunk->lines[i+1] >= 0 && i < chunk->lcount) {
		i += 2;
	} 
	// must undo the last -2
	return chunk->lines[i - 2];
} 

int addConstant(Chunk* chunk, Value value) {
    push(value);
	writeValueArray(&chunk->constants, value);
    pop();

	// return index where the constant was appended
	return chunk->constants.count - 1;
} 
