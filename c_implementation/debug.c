#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
	printf("== %s ==\n", name); // a small header
	for(int offset = 0; offset < chunk->count;) 
		// disassemble instruction function increments offset as needed
		// since some instructions will be larger than others
		offset = disassembleInstruction(chunk, offset);
} 

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
	uint8_t constant = chunk->code[offset+1]; // get the index of the data
	printf("%-16s %4d '", name, constant);
	printValue(chunk->constants.values[constant]);
	printf("'\n");
	return offset + 2;
} 

static int constantLongInstruction(const char* name, Chunk* chunk, int offset) {
	// uint32_t constant = chunk->code[offset] & 0x00FFFFFF; // get the last 24 bits in there
	uint32_t constant = (chunk->code[offset + 1] << 16) | (chunk->code[offset + 2] << 8) | (chunk->code[offset + 3]);
	constant &= 0x00FFFFFF;
	printf("%-16s %4d '", name, constant);
	printValue(chunk->constants.values[constant]);
	printf("'\n");
	return offset + 4;
} 

static int simpleInstruction(const char* name, int offset) {
	printf("%s\n", name);
	return offset + 1;
} 

int disassembleInstruction(Chunk* chunk, int offset) {
	printf("%04d ", offset);

	//i.e. if the lines are the same, don't make a brand new line
	// if(offset > 0 && chunk->lines[offset] == chunk->lines[offset-1])
	if(offset > 0 && getLine(chunk, offset) == getLine(chunk, offset-1))
		printf("   | ");
	else 
		printf("%4d ", getLine(chunk, offset));

	uint8_t instruction = chunk->code[offset];
	switch(instruction) {
		case OP_CONSTANT:
			return constantInstruction("OP_CONSTANT", chunk, offset);
		case OP_CONSTANT_LONG:
			return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
		case OP_NIL:
			return simpleInstruction("OP_NIL", offset);
		case OP_TRUE:
			return simpleInstruction("OP_TRUE", offset);
		case OP_FALSE:
			return simpleInstruction("OP_FALSE", offset);
		case OP_EQUAL:
			return simpleInstruction("OP_EQUAL", offset);
		case OP_GREATER:
			return simpleInstruction("OP_GREATER", offset);
		case OP_LESS:
			return simpleInstruction("OP_LESS", offset);
		case OP_ADD:
			return simpleInstruction("OP_ADD", offset);
		case OP_SUBTRACT:
			return simpleInstruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:
			return simpleInstruction("OP_MULTIPLY", offset);
		case OP_DIVIDE:
			return simpleInstruction("OP_DIVIDE", offset);
		case OP_NEGATE:
			return simpleInstruction("OP_NEGATE", offset);
		case OP_NOT:
			return simpleInstruction("OP_NOT", offset);
		case OP_PRINT:
			return simpleInstruction("OP_PRINT", offset);
		case OP_RETURN:
			return simpleInstruction("OP_RETURN", offset);
		case OP_POP:
			return simpleInstruction("OP_POP", offset);
		case OP_DEFINE_GLOBAL:
			return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
		case OP_DEFINE_GLOBAL_LONG:
			return constantLongInstruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
		case OP_GET_GLOBAL:
			return constantInstruction("OP_GET_GLOBAL", chunk, offset);
		case OP_GET_GLOBAL_LONG:
			return constantInstruction("OP_GET_GLOBAL_LONG", chunk, offset);
		case OP_SET_GLOBAL:
			return constantInstruction("OP_SET_GLOBAL", chunk, offset);
		case OP_SET_GLOBAL_LONG:
			return constantInstruction("OP_SET_GLOBAL_LONG", chunk, offset);
		default:
			printf("Unknown opcode %d\n", instruction);
			return offset+1;
	} 
} 
