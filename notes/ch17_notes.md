# Ch. 17: Compiling Expressions

- in this chapter, 3 big things!
    - final segment of VM execution pipeline
    - writing a compiler that parses source code and outputs instructions
        - well, bytecode, but still
    - special algorithm: Vaughan Pratt's "top-down operator precedence parsing"
        - apparently, some sort of oral tradition
- first, we let the compiler get a chunk for the compiler to write to
    - it will return if the compilation was successful
- an initial `advance()` is done to "prime the pump" on the scanner
    - only supports single expression parsing for now

## Single-pass compilation

- many languages split the roles of *parsing the code* and *outputting instructions* into different passes in the implementation of a compiler
    - ex. parser produces AST, code generator outputs target code
- clox is instead merging these passes into one to keep the compiler simple
    - this doesn't work well for all languages...you can't have too much surrounding stuff for a peice of syntax

## Parsing tokens

- add global variable `parser` to handle the parsing, with a `hadError` field, but also a `panicMode` flag (to mimic exceptions)

## Emitting bytecode

- translate the parsed syntax to bytecode
- simple writing to the chunk for now

## Parsing prefix expression

- missing the connection between the parsing and the code generation
- focus on just number literals, parens, unary negation, and arithmetic

### Parsers for tokens

- map each token type to a different kind of expression (interesting data structure here)
    - define a function for each expression that outputs appropriate bytecode
    - build an array of function pointers
    - indexes of the array correspond to the `TokenType` enum values
    - functions at each index is the code to compile an expression of that type

### Parentheses for grouping

- most expressions are more than a single token long, though...
    - expressions that *start* with a particular token are called **prefix expressions**
    - this is totally possible in C though, and can take advantage of recursion just like the java one did
> Pratt parsers are recursive, just not recursive *descent* parsers.
- grouping expressions are solely syntatic and allow lower precedence stuff higher
    - has no runtime semantics and does not emit any bytecode
    - the inner call to `expression` takes care of that

### Unary negation

- the bytecode for the negation is emitted after the operand, because the operand must be evaluated first
    - this may lead to a weird bug with a multiline subtraction, because the current token when the bytecode is written is *not* the `-` token
    - a more robust approach may store the token's line before compiling and pass it into `emitByte`...which I can do...
- one small problem: `expression` calls with any precedence, which is bad...
    - what about `-a.b + c`? In this case, `-` should just apply to `a.b`, not `a.b + c`, so it must recursively go into something of higher precedence
- this is strange though...jlox accomplished this by calling into the parsing method for the lowest-allowed-precedence expression
    - each method for parsing a specific expression also parsed any expressions of higher precedence too
- different story here...each function only parses *exactly one* type of expression, and does not cascade at all
    - must handle the precedence separately
- the `parsePrecedence` function starts at the current token and parses any expression at the given precedence level or higher

### Parsing infix expressions

- binary operators are *infix* (like `a + b` instead of `a b +`
- in parsing `1 + 2`:
    - call `expression()` $\rightarrow$ calls `parsePrecedence(PREC_ASSIGNMENT)`
    - sees the number and goes to `number()`
    - `number()` gives a constant to the chunk (`OP_CONSTANT`) and returns to `parsePrecedence`
- so `parsePrecedence` needs to consume the addition expression and eat the `+`
- our array of function pointers is instead a *table* of function pointers
    - one col for prefix parser functions, one column for infix parser functions
- when an infix parser function is called, the left-hand operand is already compiled
- notice that we parse with one level higher than this operator's level because binary operators are *right-associative*
    - we parse `1 + 2 + 3 + 4` as `((1 + 2) + 3) + 4`
    - assignment, `a = b = c = d`, is right associative: `a = (b = (c = d))`
        - to do this, we call `parsePrecedence` with the *same* precedence as the current one
    - so, we can always just add one to the current precedence

## A Pratt Parser

- we have parts of the compiler laid out, but need to implement some of the hanging functions
    - also, we need a table where given a TokenType,
        - we can get a function to compile a prefix expression with a token of that type
        - a function to compile an infix expression whose left operand is followed by a token of that type,
        - the precedence of an infix expression that uses that token as an operator

### Parsing with precedence

- parsing with precedence is basically done by `parsePrecedence`
- the function is very clever: it first sees if the token is a prefix operator (i.e. if it begins some sort of expression) and does that first
    - then, it looks for the infix operator in a table and does that, of higher priority
    - loops that until done, then it starts over

## Dumping chunks

- add support for dumping chunk (seeing print output)
- omg it works

## Challenges

1. write a trace of what happens when parsing:
```
(-1 + 2) * 3 - -4
```
    - the compiler calls `expression`, which calls `parsePrecedence`
    - `parsePrecedence` reads a `(`, calls `grouping` from the table (received from `getRule`)
    - `grouping` calls `parsePrecedence` again via `expression`
    - `parsePrecedence` eats the `-`, calls `unary()` from the table
    - `unary()` calls `parsePrecedence` again
    - `parsePrecedence` reads the 1, calls `number`, which spits out a 1 onto the stack 
        - no other expressions called because of high precedence
    - `parsePrecedence` continues and calls `binary()` with `ADD` as its previous token
    - `binary()` sees the type of token, calls `parsePrecedence`, which executes at higher precedence and thus eats and spits out a 2 as above, and puts the bytecode for an add instruction in the chunk
        - this `parsePrecedence` saw that the `)` had no infix operator associated with it and returned (its precedence is set as lowest)
    - the `parsePrecedence` which called `binary()` now returns, and the `grouping` eats the righthand `)` and returns
    - thus, the `parsePrecedence` which ate the `(` now continues and reads the `*`, and starts another `binary()` call
    - this likewise reads the `3` and spits it onto the stack from another call to `parsePrecedence`, and then puts on a multiply instruction, and returns
    - then the `parsePrecedence` which called `binary` goes to the next loop, as this was the initial one, and reads the `-`, so it calls `binary()` again
    - then this one reads the `-` and calls `unary()`
    - that `unary()` calls `number` (indirectly), which spits out the bytecode to go on the stack, and returns after emitting bytecode for a negate
    - then that `parsePrecedence` returns, back to the one which called `unary` on this `-`, andit returns too
    - then the `binary` emits the proper bytecode for subtract and returns
    - and then everything returns out since we've reached EOF
