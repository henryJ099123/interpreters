#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    PREC_ASSIGNMENT_TERNARY, // =
    PREC_OR, // or
    PREC_AND, // and
    PREC_EQUALITY, // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM, // + -
    PREC_FACTOR, // * /
    PREC_UNARY, // ! -
    PREC_CALL, // . ()
    PREC_PRIMARY
} Precedence;

// all parse functions must take nothing and return nothing
typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name; // name of variable
    int depth; // scope depth of block where local is declared
} Local;

// a flat array of all locals in scope at each point in compilation
// ordered in array in order of code declaration
typedef struct {
    Local locals[UINT8_COUNT];
    int localCount; // number of locals in scope
    int scopeDepth; // number of blocks surrounding the current bit of code
} Compiler;

// global variables! Love it!
Parser parser;
Compiler* current = NULL;
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

static bool check(TokenType type) {
    return parser.current.type == type;
} 

// if it matches, we consume; otherwise we return false
static bool match(TokenType type) {
    if(!check(type)) return false;
    advance();
    return true;
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

static void emitLongIndex(int constant) {
    writeLongConstant(currentChunk(), constant, parser.previous.line);
} 

static void emitReturn() {
    emitByte(OP_RETURN);
} 

// find some way to optimize this for 1 byte or 3 bytes instead of always 4
static int makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    return constant;
}

static void emitConstant(Value value) {
    writeConstant(currentChunk(), value, parser.previous.line);
} 

static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
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

static void beginScope() {
    current->scopeDepth++;
} 

static void endScope() {
    current->scopeDepth--;
    while(current->localCount > 0 &&
          current->locals[current->localCount - 1].depth > current->scopeDepth) {
        // remove local variable from the stack
        emitByte(OP_POP);
        // decrement number of locals
        current->localCount--;
    } 
} 

// must forward define these because they are used recursively
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void literal(bool canAssign) {
    //parsePrecedence has already eaten the keyword token
    switch(parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return; // unreachable
    } 
} 

static void binary(bool canAssign) {
    // get the operator
    TokenType operatorType = parser.previous.type;
    // get the rule associated with this operator...cannot access table directly
    ParseRule* rule = getRule(operatorType);
    // execute the righthand side
    // gets the rule's precedence
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch(operatorType) {
        // some of these are desugarized
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return; // should not be reached
    } 
} 

static void ternary(bool canAssign) {
    // right now, this must be the '?'
    // TokenType operatorType = parser.previous.type;
    parsePrecedence(PREC_ASSIGNMENT_TERNARY);
    // perhaps some byte emission?
    consume(TOKEN_COLON, "Expect ':' after '?' in ternary expression.");
    parsePrecedence(PREC_ASSIGNMENT_TERNARY);
    // this is where some byte emission would occur

} 

static void grouping(bool canAssign) {
    // assume the '(' was already consumed
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
} 

// store pointer to the following function at `TOKEN_NUMBER` index in array
static void number(bool canAssign) {
    // assume the token for the number already consumed and stored in previous
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
} 

static void string(bool canAssign) {
    // + 1 to avoid the opening "
    // - 2 to avoid the ending " (and maybe the \0 null char?)
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
} 

// will put the token's name on the Value array and puts its index on the code chunk 
static int identifierConstant(Token* name) {
    
    // TODO: These changes result in memory problems with the garbage collector
    // currently there is *memory leakage* if the string *is* found already because 
    // the string is just thrown away (variable_name)
    ObjString* variable_name = copyString(name->start, name->length);
    Value index;
    if(tableGet(&vm.globalNames, variable_name, &index)) {
        return (int) AS_NUMBER(index);
    }
    int newIndex = vm.globalValues.count;
    writeValueArray(&vm.globalValues, UNDEF_VAL);
    tableSet(&vm.globalNames, variable_name, NUMBER_VAL((double) newIndex));
    return newIndex;
    // return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
} 

// simple check of two strings
// tokens are not LoxStrings so their hashes have not been calculated
static bool identifiersEqual(Token* a, Token* b) {
    if(a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
} 

// for each spot on the locals array, we look for the variable
// and walk it backwards for shadowing of variables
static int resolveLocal(Compiler* compiler, Token* name) {
    for(int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if(identifiersEqual(name, &local->name)) {
            if(local->depth == -1)
                error("Can't read local variable in its own initializer.");
            return i;
        } 
    } 
    return -1;
} 

// initializes the next available local in the compiler's array of variables
static void addLocal(Token name) {
    if(current->localCount == UINT8_COUNT) {
        error("Too many local variables in this scope.");
        return;
    } 
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
} 

// global variables are late bound and thus ignored here
// local variables are remembered
static void declareVariable() {
    if(current->scopeDepth == 0) return;
    Token* name = &parser.previous;

    // current scope is end of array, so we move backwards looking for a match
    for(int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        // if we've gone out a scope then we're done
        if(local->depth != -1 && local->depth < current->scopeDepth)
            break;
        // if there's one *in the same scope* we throw error
        if(identifiersEqual(name, &local->name))
            error("Already a variable  with this name in this scope.");
    } 
    addLocal(*name);
} 

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);

    if(arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else {
        // reassigns *arg* to point to the table from before
        int arg = identifierConstant(&name);
        getOp = arg <= UINT8_MAX ? OP_GET_GLOBAL : OP_GET_GLOBAL_LONG;
        setOp = arg <= UINT8_MAX ? OP_SET_GLOBAL : OP_SET_GLOBAL_LONG;
    } 

    if(canAssign && match(TOKEN_EQUAL)) {
        expression();
        // set expression.
        emitByte(setOp);
    } else {
        // get expression.
        emitByte(getOp);
    } 

    if(getOp == OP_GET_GLOBAL_LONG) emitLongIndex(arg);
    else emitByte((uint8_t) arg);
} 

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
} 

static void unary(bool canAssign) {
    // get the operator from beforehand
    TokenType operatorType = parser.previous.type;

    // Compile the operand
    // must negate the value after the operand's bytecode...because you need the operand to negate it
    // permits nested unary expressions
    parsePrecedence(PREC_UNARY);

    switch(operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        case TOKEN_BANG: emitByte(OP_NOT); break;
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
    [TOKEN_QUESTION]      = {NULL,     ternary, PREC_ASSIGNMENT_TERNARY},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
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
    bool canAssign = precedence <= PREC_ASSIGNMENT_TERNARY;
    prefixRule(canAssign);

    // now we look for an infix expression
    // but it must be of an equal or higher priority than what we have
    // if the next token isn't an infix operator, this doesn't get called?
    // it must be because the current token's precedence is either NONE
    // or below where it should be if it shouldn't be called
    while(precedence <= getRule(parser.current.type)->precedence) {
        advance();
        // get function and call it
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        // canAssign passed here for setters
        infixRule(canAssign);
    } 

    if(canAssign && match(TOKEN_EQUAL)) error("Invalid assignment target.");
} 

// requires next token to be an identifier
static int parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();
    
    // exit function if in local scope, so dummy table index returned
    if(current->scopeDepth > 0) return 0;
    return identifierConstant(&parser.previous);
} 

static void markInitialized() {
    // change most recent local variable to have the correct scope
    current->locals[current->localCount - 1].depth = current->scopeDepth;
} 

static void defineVariable(int global) {
    // no code to declare local variable at runtime
    // statically resolved
    if(current->scopeDepth > 0) {
        markInitialized();
        return;  
    } 
    if(global <= UINT8_MAX) {
        emitBytes(OP_DEFINE_GLOBAL, (uint8_t) global);
        return;
    } 
    emitByte(OP_DEFINE_GLOBAL_LONG);
    emitLongIndex(global);
} 

// returns a pointer to the ParseRule at given TokenType
static ParseRule* getRule(TokenType type) {
    return &rules[type];
} 

static void synchronize() {
    parser.panicMode = false;
    // skip tokens until we reach a statement boundary
    // this can be the ';' or the start of some statement
    while(parser.current.type != TOKEN_EOF) {
        if(parser.previous.type == TOKEN_SEMICOLON) return;
        switch(parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default: ;
        } 
        advance();
    } 
} 

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT_TERNARY); // highest level  
} 

static void block() {
    while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
        declaration();
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
} 

static void varDeclaration() {
    int global = parseVariable("Expect variable name.");
    // if there's an equal sign, we have to assign the thing
    if(match(TOKEN_EQUAL)) expression();
    // otherwise, we return nil for it to attach
    else emitByte(OP_NIL);
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
} 

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after print statement.");
    emitByte(OP_PRINT);
} 

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression statement.");
    emitByte(OP_POP);
} 

static void declaration() {
    if(match(TOKEN_VAR)) varDeclaration();
    else statement();
    if(parser.panicMode) synchronize();
} 

static void statement() {
    if(match(TOKEN_PRINT)) printStatement();
    else if(match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } 
    else expressionStatement();
} 

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    Compiler compiler; // same as the "current" field
    initCompiler(&compiler);
    compilingChunk = chunk;

    // init the parser
    parser.hadError = false;
    parser.panicMode = false;

    // apparently "primes the pump" of the scanner
    advance();
    // only expressions for now
    while(!match(TOKEN_EOF))
        declaration();

    endCompiler();
    return !parser.hadError;
} 
