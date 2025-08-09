#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
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
    bool modifiable; // for mutable vs. immutable variables
    bool isCaptured; // for if a closure got it
} Local;

typedef struct {
    uint8_t index; // stores local slot the upvalue is capturing
    bool isLocal;
} Upvalue;

// tiny enum
typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT
} FunctionType;

// a flat array of all locals in scope at each point in compilation
// ordered in array in order of code declaration
typedef struct Compiler {
    struct Compiler* enclosing;
    // reference to a function being built
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount; // number of locals in scope
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth; // number of blocks surrounding the current bit of code

    // stuff for break and continue
    int breakLocations[UINT8_COUNT];
    uint8_t breakCount;
    int loopDepth;
    int currentLoopScope;
    int continueJumpLocation;
    int breakJumpLocation;
} Compiler;

// global variables! Love it!
Parser parser;
Compiler* current = NULL;
// Chunk* compilingChunk;

// current chunk: the one owned by the function we compile
static Chunk* currentChunk() {
    return &current->function->chunk;
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

static void emitByteAndIndex(uint8_t byteShort, uint8_t byteLong, Value value) {
    int constant = addConstant(currentChunk(), value);
    if(constant <= UINT8_MAX) {
        emitBytes(byteShort, (uint8_t) constant);
    } else {
        emitByte(byteLong);
        emitLongIndex(constant);
    } 
} 

// emits a new loop instruction that unconditionally jumps backwards
static void emitLoop(int loopStart) {
    // OP_LOOP goes backwards
    emitByte(OP_LOOP);
    int offset = currentChunk()->count - loopStart + 2;
    if(offset > UINT16_MAX) error("Loop body is too large.");
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
} 

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff); // this one's location returned
    emitByte(0xff);
    // emits an int of the location of the second emitByte here
    return currentChunk()->count - 2;
} 

static void emitReturn() {
    // implicit `nil` when there is not a return statement
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
} 

/*
// find some way to optimize this for 1 byte or 3 bytes instead of always 4
static int makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    return constant;
}
*/

static void emitConstant(Value value) {
    writeConstant(currentChunk(), value, parser.previous.line);
} 

static void patchJump(int offset) {
    // ...why do the subtract 2's at all?
    int jump = currentChunk()->count - offset - 2;
    if(jump > UINT16_MAX)
        error("Jump distance is too far.");

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
} 

static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;

    // locals
    compiler->localCount = 0;
    compiler->scopeDepth = 0;

    compiler->function = newFunction();
    
    // stuff for break and continue
    compiler->loopDepth = 0;
    compiler->currentLoopScope = -1;
    compiler->continueJumpLocation = -1;
    compiler->breakJumpLocation = -1;
    compiler->breakCount = 0;
    current = compiler;

    if(type != TYPE_SCRIPT) {
        // we *copy* the name string because we may not keep access to the original source
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    } 

    // claims slot 0 of the locals array for the VM's internal use
    Local* local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false; // will be capturable by the user later!
    local->name.start = "";
    local->name.length = 0;
} 

static int getScope() {
    return current->scopeDepth;
}

static ObjFunction* endCompiler() {
    // temp
    emitReturn();
    ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
    // only dump if error-free
    if(!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL
            ? function->name->chars: "<script>");
    }
#endif

    current = current->enclosing;
    return function;
} 

static void beginScope() {
    current->scopeDepth++;
} 

static void endScope() {
    current->scopeDepth--;
    while(current->localCount > 0 &&
          current->locals[current->localCount - 1].depth > current->scopeDepth) {
        // remove local variable from the stack
        if(current->locals[current->localCount - 1].isCaptured)
            // tells runtime exactly when a local must move to the heap
            emitByte(OP_CLOSE_UPVALUE);
        else
            emitByte(OP_POP);
        // decrement number of locals
        current->localCount--;
    } 
} 

static void emitPopsForLocals(int scope) {
    for(int i = current->localCount - 1; i >= 0 && current->locals[i].depth > scope; i--) {
        // remove local variable from the stack, but not from the compiler
        emitByte(OP_POP);
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

// put all arguments on the stack
static uint8_t argumentList() {
    uint8_t argCount = 0;
    if(!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if(argCount == 255)
                error("Can't have more than 255 arguments.");
            argCount++;
        } while(match(TOKEN_COMMA));
    } 
    consume(TOKEN_RIGHT_PAREN, "Expect ')' to end function call.");
    return argCount;
} 

// function already on top of stack as a `get` from the local variable
static void call(bool canAssign) {
    // compiles the arguments
    uint8_t argCount = argumentList();
    // invoke function and use argument count as an operand
    emitBytes(OP_CALL, argCount);
} 

static void ternary(bool canAssign) {
    // right now, this must be the '?'
    // TokenType operatorType = parser.previous.type;

    // include a jump to the false bit if this is false
    int skipTrue = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // pop for the true one

    parsePrecedence(PREC_ASSIGNMENT_TERNARY);

    // skip the false bit always
    int skipFalse = emitJump(OP_JUMP);
    patchJump(skipTrue);
    emitByte(OP_POP); // pop for the false one

    consume(TOKEN_COLON, "Expect ':' after '?' in ternary expression.");
    parsePrecedence(PREC_ASSIGNMENT_TERNARY);

    patchJump(skipFalse);

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

// TODO memory leak here as well
static bool isGlobalConstant(Token* name) {
    ObjString* variable_name = copyString(name->start, name->length);
    return tableKeyExists(&constantGlobals, variable_name);
} 

// will put the token's name on the Value array and puts its index on the code chunk 
static int identifierConstant(Token* name, bool modifiable) {
    
    // TODO: These changes result in memory problems with the garbage collector
    // currently there is *memory leakage* if the string *is* found already because 
    // the string is just thrown away (variable_name)
    ObjString* variable_name = copyString(name->start, name->length);
    // need to push this variable to keep it alive until added to these tables
    push(OBJ_VAL(variable_name));
    Value index;
    if(!modifiable)
        tableSet(&constantGlobals, variable_name, NIL_VAL);
    if(tableGet(&vm.globalNames, variable_name, &index)) {
        pop();
        return (int) AS_NUMBER(index);
    }
    int newIndex = vm.globalValues.count;
    writeValueArray(&vm.globalValues, UNDEF_VAL);
    tableSet(&vm.globalNames, variable_name, NUMBER_VAL((double) newIndex));
    pop();
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
static int resolveLocal(Compiler* compiler, Token* name, bool* modifiable) {
    for(int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if(identifiersEqual(name, &local->name)) {
            if(local->depth == -1)
                error("Can't read local variable in its own initializer.");
            *modifiable = local->modifiable;
            return i;
        } 
    } 
    return -1;
} 

// create an upvalue to add to compiler
// indices in the compiler's array here match the indices where upvalues will be
// in the closure at runtime
static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    // return upvalue index if it already exists
    for(int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->isLocal == isLocal)
            return i;
    } 

    if(upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in this function.");
        return 0;
    } 

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    // closure tracks which variable in enclosing function must be captured
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
} 

static int resolveUpvalue(Compiler* compiler, Token* name, bool* modifiable) {
    // if this is top-level function stop here
    // another base-case
    if(compiler->enclosing == NULL) return -1;

    // otherwise find this local variable in the scope just outside

    int local = resolveLocal(compiler->enclosing, name, modifiable);

    // first look for a matching local variable in the enclosing function
    // if it is there, we capture that local and return
    if(local != -1) {
        // this local has become captured
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t) local, true); // this *was* just outside
    }

    // we have recursive action both before and after the call
    // look for local beyond immediately enclosing function
    // this call will eventually end in the above case,
    // so this value here will be the *upvalue* for that local
    int upvalue = resolveUpvalue(compiler->enclosing, name, modifiable);
   
    // if an outer-outer function's local was captured,
    // we take the index for that upvalue and add it to this compiler
    if(upvalue != -1)
        return addUpvalue(compiler, (uint8_t) upvalue, false); // this was *not* just outside
    
    return -1;
} 

// initializes the next available local in the compiler's array of variables
static void addLocal(Token name, bool modifiable) {
    if(current->localCount == UINT8_COUNT) {
        error("Too many local variables in this scope.");
        return;
    } 
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->modifiable = modifiable;
    local->isCaptured = false;
} 

// global variables are late bound and thus ignored here
// local variables are remembered
static void declareVariable(bool modifiable) {
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
            error("Already a variable or constant with this name in this scope.");
    } 
    addLocal(*name, modifiable);
} 

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    bool modifiable = true;
    int arg = resolveLocal(current, &name, &modifiable);

    if(arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if((arg = resolveUpvalue(current, &name, &modifiable)) != -1) {
        // could be the case this is local variable of an enclosing function
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } 
    else {
        // reassigns *arg* to point to the table from before
        // as this is not defining anything, we don't want to mess with the global const table
        arg = identifierConstant(&name, true);
        modifiable = !isGlobalConstant(&name);
        getOp = arg <= UINT8_MAX ? OP_GET_GLOBAL : OP_GET_GLOBAL_LONG;
        setOp = arg <= UINT8_MAX ? OP_SET_GLOBAL : OP_SET_GLOBAL_LONG;
    } 

    if(canAssign && match(TOKEN_EQUAL)) {
        if(!modifiable) {
            // keep in mind: this *won't* evaluate the rest of the expression
            error("Cannot assign to a constant.");
            return;
        } 
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

// this is an infix operator function
static void and_(bool canAssign) {
    // left hand side already on top
    // checks what's on top. if it's false we skip
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    // if it's true we discard the left-hand side value
    emitByte(OP_POP);

    // this becomes the value of the whole `and` expression
    parsePrecedence(PREC_AND);

    patchJump(endJump);
} 

// bad way to do this. just add a JUMP_IF_TRUE thing
static void or_(bool canAssign) {
    // if this one is false, we skip to the next side
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    // i.e. if it was true, then we have a jump to skip the right-hand operand
    int endJump = emitJump(OP_JUMP);

    // jump here (past the previous jump)
    patchJump(elseJump);
    // pop the lefthand side as it isn't important
    emitByte(OP_POP);

    // continue the expression
    parsePrecedence(PREC_OR);

    // jump here if it was true
    patchJump(endJump);
} 

// table of rules
// the [TOKEN_DOT] syntax helps initialize the exact indices of an array, I think
// very easy to see which tokens have expressions associated with them!
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COLON]         = {NULL,     NULL,   PREC_NONE},
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
    [TOKEN_AND]           = {NULL,     and_,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     or_,    PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SWITCH]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CASE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DEFAULT]       = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CONST]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CONTINUE]      = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BREAK]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
    advance();
    // if no prefix parser, this token must be a syntax error,
    // i.e. it should not be placed here
    // reading L to R < the first token is *always* a prefix expression
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if(parser.previous.type == TOKEN_CASE) {
        error("Can't have 'case' after 'default' or outside of a 'switch'.");
        return;
    } 
    if(parser.previous.type == TOKEN_DEFAULT) {
        error("Can't have 'default' outside of a 'switch'.");
        return;
    } 
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
static int parseVariable(const char* errorMessage, bool modifiable) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable(modifiable);
    
    // exit function if in local scope, so dummy table index returned
    if(current->scopeDepth > 0) return 0;
    return identifierConstant(&parser.previous, modifiable);
} 

static void markInitialized() {
    // change most recent local variable to have the correct scope
    if(current->scopeDepth == 0) return;
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

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    // parameter is just a local variable declared in outermost lexical scope
    // no code to initialize the parameter's value...that's done at call-time
    if(!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if(current->function->arity > 255)
                errorAtCurrent("Can't have more than 255 parameters.");
            uint8_t constant = parseVariable("Expect parameter name.", true);
            defineVariable(constant);
        } while(match(TOKEN_COMMA)); 
    } 
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    // no `endScope()` call because the Compiler is ended completely at the end of the body
    ObjFunction* function = endCompiler();

    emitByteAndIndex(OP_CLOSURE, OP_CLOSURE_LONG, OBJ_VAL(function));

    // has variable size encoding
    // for each upvalue, we have two single-byte operands that specify what the upvalue captures
    for(int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? : 0);
        emitByte(compiler.upvalues[i].index);
    } 
}

static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name.", true);
    markInitialized();
    function(TYPE_FUNCTION);
    
    // top level function declaration bound to global variable
    defineVariable(global);
}

static void varDeclaration(bool modifiable) {
    int global = parseVariable("Expect variable name.", modifiable);
    // if there's an equal sign, we have to assign the thing
    if(match(TOKEN_EQUAL)) expression();
    // otherwise, we return nil for it to attach
    else if(!modifiable) {
        error("Must initialize a constant with a value at declaration.");
    } 
    else emitByte(OP_NIL);
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
} 

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after print statement.");
    emitByte(OP_PRINT);
} 

static void returnStatement() {
    if(current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    } 
    // if no return value we emit a return, which implicitly handles the NIL
    if(match(TOKEN_SEMICOLON))
        emitReturn();
    // otherwise we leave the return value on the stack
    else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
} 

static void continueStatement() {
    if(current->loopDepth == 0) {
        error("Can't have 'continue' outside of a loop.");
        return;
    } 
    consume(TOKEN_SEMICOLON, "Expect ';' after continue statement.");
    // end as many scopes as needed to bust out of the loop
    emitPopsForLocals(current->currentLoopScope);
//  printf("where we jumpin: %d\n----\n", current->continueJumpLocation);
    emitLoop(current->continueJumpLocation);
} 

static void breakStatement() {
    if(current->loopDepth == 0) {
        error("Can't have 'break' outside of a loop.");
        return;
    } 
    consume(TOKEN_SEMICOLON, "Expect ';' after break statement.");
    if(current->breakCount == UINT8_MAX) {
        error("Too many break statements in this scope.");
        return;
    } 
    emitPopsForLocals(current->currentLoopScope);
    current->breakLocations[current->breakCount] = emitJump(OP_JUMP);    
    current->breakCount++;
} 

static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition of 'while'.");

    // jump out of while loop
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // remove condition from the stack

    // metadata for continue and break statements
    int oldLoopScope = current->currentLoopScope;
    int oldContinueJump = current->continueJumpLocation;
    uint8_t numBreaks = current->breakCount;
    current->currentLoopScope = getScope();
    current->loopDepth++;
    current->continueJumpLocation = loopStart;
    // body
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP); // remove condition

    for(int i = 0; i < current->breakCount; i++)
        printf("break loc: %d\n", current->breakLocations[i]);

    for(; current->breakCount > numBreaks; current->breakCount--)
        patchJump(current->breakLocations[current->breakCount - 1]);

    // we decrement the loop depth.
    // take advantage of stack call here.
    current->currentLoopScope = oldLoopScope;
    current->continueJumpLocation = oldContinueJump;
    current->loopDepth--;
} 

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression statement.");
    emitByte(OP_POP);
} 

static void forStatement() {
    beginScope(); // for the variable declaration
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    
    // initializer.
    if(match(TOKEN_SEMICOLON)) {
        // no initializer
    } else if(match(TOKEN_VAR)) varDeclaration(true);
    else expressionStatement(); // doesn't *need* to be a variable declaration
    // consume(TOKEN_SEMICOLON, "Expect ';' after 'for' initializer.");

    int loopStart = currentChunk()->count;
    int exitJump = -1; // dummy value
    if(!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // jump out of loop
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // remove condition
    } 

    if(!match(TOKEN_RIGHT_PAREN)) {
        // we first jump over the increment clause's code to the loop body
        int bodyJump = emitJump(OP_JUMP); 

        // record location of the start of the increment
        int incrementStart = currentChunk()->count; 
        expression(); 
        emitByte(OP_POP); 
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'for' clauses.");

        // then we put a jump back to before the condition
        emitLoop(loopStart); 
        loopStart = incrementStart; 
        patchJump(bodyJump); 
        
        // flow looks like this:
        // condition => jump over increment => loop body => jump to increment => jump to condition
    } 
    // metadata for continue and break statements
    int oldLoopScope = current->currentLoopScope;
    int oldContinueJump = current->continueJumpLocation;
    uint8_t numBreaks = current->breakCount;
    current->currentLoopScope = getScope();
    current->loopDepth++;
    current->continueJumpLocation = loopStart;

    statement();

    // if there's an increment, this will jump back there
    emitLoop(loopStart);

    // only patch the jump if there's a condition clause
    // no condition ==> no jump to patch ==> no condition value on top of the stack
    // i.e. no getting out of the loop
    if(exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // remove condition
    } 
    
    for(; current->breakCount > numBreaks; current->breakCount--)
        patchJump(current->breakLocations[current->breakCount - 1]);

    // we decrement the loop depth.
    // take advantage of stack call here.
    current->currentLoopScope = oldLoopScope;
    current->loopDepth--;
    current->continueJumpLocation = oldContinueJump;

    endScope();
} 

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition of 'if' statement.");
    
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // remove condition. Won't be taken if the jump is false
    statement();

    int elseJump = emitJump(OP_JUMP);
    
    // always do the then jump even if it isn't needed?
    patchJump(thenJump);
    emitByte(OP_POP); // remove condition. Won't be taken if the jump is true

    if(match(TOKEN_ELSE)) statement();

    // fixes the jump instruction for *right here*
    patchJump(elseJump);
} 

static void switchStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
    // this is the condition for the switch.
    // we need this *a lot*, but we *cannot* jump back and forth
    // without a TON of crazy jumps, because the jump dist changes each time.
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition of 'switch' statement.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' to start 'switch' body.");

    // specific jumps.
    int jumpToNextCase = 0;
    int jumpOut = 0;
    bool first_case = true;
    // we're in a 'case' for the switch.
    while(match(TOKEN_CASE)) {
        // we first *duplicate* the expression's final value.
        // this is how we keep the switch condition.
        emitByte(OP_DUP);
        // this will add the case's expression on the stack
        expression();
        consume(TOKEN_COLON, "Expect ':' after 'case'.");
        // this will put whether the two things are equal on the stack
        emitByte(OP_EQUAL);

        // if they're unequal, we jump to the next case (see `if` above)
        jumpToNextCase = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // pop the condition if it was true
        
        // add statements
        while(!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) &&
              !check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) statement();
        
        // for the jumpOut, I didn't want to allocate a list of integers,
        // so instead if the case is done, it'll keep jumping to the next case
        // to the end. It's a bit slower but smaller complexity.
        if(!first_case)
            patchJump(jumpOut);
        jumpOut = emitJump(OP_JUMP);

        // this preps the next case (or when there's no case, the exit)
        
        // we add a jump here for when the previous case fails
        // we also remove the bool value sitting on the stack
        patchJump(jumpToNextCase);
        emitByte(OP_POP);
        first_case = false;
    } 

    // if there's a default:
    if(match(TOKEN_DEFAULT)) {
        consume(TOKEN_COLON, "Expect ':' after 'default'.");
        // we patch the jump again to get here
        // add statements until the }, as there aren't other cases after the default
        while(!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) statement();
    } 

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after 'switch' body.");
    // there's no jump to patch if there's no cases at all
    if(!first_case) {
        patchJump(jumpOut);
        emitByte(OP_POP); // to remove the continually-duplicated condition
    }
} 

static void declaration() {
    if(match(TOKEN_FUN)) funDeclaration();
    else if(match(TOKEN_VAR)) varDeclaration(true);
    else if(match(TOKEN_CONST)) varDeclaration(false);
    else statement();
    if(parser.panicMode) synchronize();
} 

static void statement() {
    if(match(TOKEN_PRINT)) printStatement();
    else if(match(TOKEN_IF)) ifStatement();
    else if(match(TOKEN_RETURN)) returnStatement();
    else if(match(TOKEN_WHILE)) whileStatement();
    else if(match(TOKEN_FOR)) forStatement();
    else if(match(TOKEN_SWITCH)) switchStatement();
    else if(match(TOKEN_CONTINUE)) continueStatement();
    else if(match(TOKEN_BREAK)) breakStatement();
    else if(match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } 
    else expressionStatement();
} 

ObjFunction* compile(const char* source) {
    initScanner(source);
    Compiler compiler; // same as the "current" field
    initCompiler(&compiler, TYPE_SCRIPT);
    initTable(&constantGlobals);
    // compilingChunk = chunk;

    // init the parser
    parser.hadError = false;
    parser.panicMode = false;

    // apparently "primes the pump" of the scanner
    advance();
    // only expressions for now
    while(!match(TOKEN_EOF))
        declaration();

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
} 

void markCompilerRoots() {
    Compiler* compiler = current;
    // the function the compiler is currently compiling
    while(compiler != NULL) {
        markObject((Obj*) compiler->function);
        compiler = compiler->enclosing;
    } 
} 
