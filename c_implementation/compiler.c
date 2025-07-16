#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
	Token current;
	Token previous;
	bool hadError;
	bool panicMode;
} Parser;

// all of the basic precedence levels
// as these are secrectly numbers we have an ascending number to represent precedence
typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_OR, // or
	PREC_AND,
	PREC_EQUALITY,
	PREC_COMPARISON,
	PREC_TERM,
	PREC_FACTOR,
	PREC_UNARY,
	PREC_CALL,
	PREC_PRIMARY
} Precedence;

// all parse functions must take nothing and return nothing
typedef void (*ParseFn)();

typedef struct {
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

// another global variable
Parser parser;
Chunk* compilingChunk;

// this will change, likely
static Chunk* currentChunk() {
	return compilingChunk;
}

/********** Error handling **********/
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
#ifdef DEBUG_PRINT_CODE
	// only dump if error-free
	if(!parser.hadError)
		disassembleChunk(currentChunk(), "code");
#endif
} 

// must forward define these because they are used recursively
static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary() {
	// get the operator
	TokenType operatorType = parser.previous.type;
	// get the rule associated with this operator...cannot access table directly
	ParseRule* rule = getRule(operatorType);
	// execute the righthand side
	// gets the rule's precedence
	parsePrecedence((Precedence)(rule->precedence + 1));

	switch(operatorType) {
		case TOKEN_PLUS: emitByte(OP_ADD); break;
		case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
		case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
		case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
		default: return; // should not be reached
	} 
} 

static void grouping() {
	// assume the '(' was already consumed
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
} 

// store pointer to the following function at `TOKEN_NUMBER` index in array
static void number() {
	// assume the token for the number already consumed and stored in previous
	double value = strtod(parser.previous.start, NULL);
	emitConstant(value);
} 

static void unary() {
	// get the operator from beforehand
	TokenType operatorType = parser.previous.type;

	// Compile the operand
	// must negate the value after the operand's bytecode...because you need the operand to negate it
	// permits nested unary expressions
	parsePrecedence(PREC_UNARY);

	switch(operatorType) {
		case TOKEN_MINUS: emitByte(OP_NEGATE); break;
		default: return; // should not happen
	} 
} 

// table of rules
// the [TOKEN_DOT] syntax helps initialize the exact indices of an array, I think
// very easy to see which tokens have expressions associated with them!
ParseRule rules[] = {
	[TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
	[TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
	[TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
	[TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
	[TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
	[TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
	[TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
	[TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
	[TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
	[TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
	[TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
	[TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
	[TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
	[TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
	[TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
	[TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
	[TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
	[TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
	[TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
	[TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
	[TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
	[TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
	[TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
	[TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
	[TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
	[TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
	[TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
	[TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
	[TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
	[TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
	[TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
	advance();
	// if no prefix parser, this token must be a syntax error,
	// i.e. it should not be placed here
	// reading L to R< the first token is *always* a prefix expression
	ParseFn prefixRule = getRule(parser.previous.type)->prefix;
	if(prefixRule == NULL) {
		error("Expect expression.");
		return;
	} 

	// parser compiles using the function it found
	prefixRule();

	// now we look for an infix expression
	// but it must be of an equal or higher priority than what we have
	// if the next token isn't an infix operator, this doesn't get called?
	// it must be because the current token's precedence is either NONE
	// or below where it should be if it shouldn't be called
	while(precedence <= getRule(parser.current.type)->precedence) {
		advance();
		// get function and call it
		ParseFn infixRule = getRule(parser.previous.type)->infix;
		infixRule();
	} 
} 

// returns a pointer to the ParseRule at given TokenType
static ParseRule* getRule(TokenType type) {
	return &rules[type];
} 

static void expression() {
	parsePrecedence(PREC_ASSIGNMENT); // highest level 	
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
