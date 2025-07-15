#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
} Parser;

// another global variable
Parser parser;
Chunk* compilingChunk;

// this will change, likely
static Chunk* currentChunk() {
	return compilingChunk;
}

static void errorAt(Token* token, const char* message) {
	// suppress any other errors while in panic mode
	if(parser.panicMode) return;
	parser.panicMode = true;
	fprintf(stderr, "[line %d] Error", token->line);
	if(token->type == TOKEN_EOF) 
		fprintf(stderr, " at end");
	else if(token->type == TOKEN_ERROR) {}
	else
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
} 

static void errorAtCurrent(const char* message) {
	errorAt(&parser.current, message);
} 

static void error(const char* message) {
	errorAt(&parser.previous, message);
} 

// step forward thru token stream
static void advance() {
	// stashes old token in previous, so we can still get it
	parser.previous = parser.current;
	for(;;) {
		// errors are done via special error tokens
		parser.current = scanToken();
		if(parser.current.type != TOKEN_ERROR) break;
		errorAtCurrent(parser.current.start);
	} 
} 

static void consume(TokenType type, const char* message) {
	// validates the expected type
	if(parser.current.type == type) {
		advance();
		return;
	} 
	errorAtCurrent(message);
} 

// append a byte to the chunk
static void emitByte(uint8_t byte) {
	// get previous line's info there for runtime errors 
	writeChunk(currentChunk(), byte, parser.previous.line);
} 

static void emitBytes(uint8_t byte1, uint8_t byte2) {
	emitByte(byte1);
	emitByte(byte2);
} 

static void emitReturn() {
	emitByte(OP_RETURN);
} 

/*
static uint8_t makeConstant(Value value) {
	writeConstant(currentChunk(), value, parser.previous.line);

	return (uint8_t) constant;
}
*/ 

static void emitConstant(Value value) {
	writeConstant(currentChunk(), value, parser.previous.line);
} 

static void endCompiler() {
	// temp
	emitReturn();
} 

// store pointer to the following function at `TOKEN_NUMBER` index in array
static void number() {
	// assume the token for the number already consumed and stored in previous
	double value = strtod(parser.previous.start, NULL);
	emitConstant(value);
} 

static void expression() {
	
} 

bool compile(const char* source, Chunk* chunk) {
	initScanner(source);
	compilingChunk = chunk;

	// init the parser
	parser.hadError = false;
	parser.panicMode = false;

	// apparently "primes the pump" of the scanner
	advance();
	// only expressions for now
	expression();
	// end the file
	consume(TOKEN_EOF, "Expect end of expression.");
	endCompiler();
	return !parser.hadError;
} 
