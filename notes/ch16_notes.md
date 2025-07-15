# Ch. 16: Scanning on Demand

- clox has a scanner, compiler, and VM
    - a data structure joins each pair of phases:
        - tokens between scanner and compiler
        - chunks (bytecode) between compiler and VM
- prepare for similarity!

## Spinning up the interpreter

- some C tricks:
    - use `fseek` to get to the end of the file
    - call `ftell` to give how many bytes from beginning
    - call `rewind` to go back to the beginning
- have to remember to check if these functions fail!
    - maybe not EVERY failure though...`fprintf` can even fail!

### Opening the compilation pipeline

- `interpret` handles the main goals of the interpreter
    - but first it gets a string of source code, which must be *compiled*

### The scanner

- scanner tracks how far it has scanned
    - but only as far as the current lexeme (`start`)

## A token at a time

- in jlox, we just had a list of tokens from scanning the whole program
    - challenge in clox...we would need a dynamic array to store all that
- however, the compiler only ever needs a few tokens, so we don't need *all* of them
    - instead, we just don't scan a token unless it is needed
    - when scanner provides the token, it is returned by value $\Rightarrow$ no dynamic allocation
- so the compiler needs to be temporarily started up
- the `.*` modifier in `printf` allows printing only a certain number of chars as desired
- added a `TOKEN_ERROR` type
    - in jlox, scanner reports errors like unterminated strings and unrecognized characters itself
    - in clox, the scanner produces a "synthetic error" token and gives it to the compiler, which can then do the error reporting
- in jlox, each Token stores the lexeme as its own Java string...which is ridiculously inefficient
    - why not use the original source string? just point to the start and to the number of chars
    - also means the source file cannot be freed until after `interpret` finishes running

### Scanning tokens

- since, in `scanToken`, the function scans a whole token, we are at the beginning of a new token
    - check if reached end of source code (look for EOF)
    - if a successful token is *not* found, then it returns an error token

## lexical grammar for Lox

- just a lot of switches and cases

### Whitespace

- whitespace does not become a part of any lexeme
    - could check for these in `scanToken` but becomes tricky to ensure the function still correctly finds the next token *after* the whitespace...need a whole loop
    - so, instead make it it's own function

### Literal tokens

- numbers and strings special because they have runtime values associated with them
    - for strings, read " and then keep reading until another "
- notice: no `literal` field in the Token struct
    - save the conversion to a runtime value later down the line
    - in clox, tokens only store the *lexeme*, not any runtime representation
    - in general, lexeme-to-value conversion in the compiler is a bit redundant, but no issues

## Identifiers and keywords

- clox recognition of keywords far different than in jlox, as we don't have an easily accessible hashmap 
    - also, a hashmap is overkill: requires walking a string to calculate a hash, then find bucket in the hash table, and do a char by char equality comparison on any string there
- instead, if we walk the string, we can quickly find out if it's a keyword by scanning the first few characters
    - start at root node of this "tree" and keep moving until we get to the end of one
    - this is literally just a deterministic finite automaton

### Tries and state machines

- this specific kind of tree, where it walks down one that it can go down, is just a finite automaton
    - tries store a set of strings, but nowhere in it is the full string
    - ecah string the trie "contains" is a *path* through the tree
    - finishing nodes are circled twice (*accept states*)
- could've made just a giant DFA for the scanner and lexical analysis
    - this is annoying by hand, Lex does it for you
- so, we're just building a way to check for ways down this tree (this is not fun coding)
- simplest code is sometimes the best!

## Challenges

1. Newer languages support **string interpolation**, which is that inside of a string literal there's special delimiters, like ${...}. What token types should be defined here? What sequence of tokens should be emitted?
    - first, need to define the `$` dollar
    - should be a new type of string/TokenType, maybe a `TOKEN_FSTRING` like Python
~~~
    - I'd make a beginner and an ender for this
    - then I'd separate anything between `${...}` as a regular string token type
    - and then 
~~~
    - a bit smarter: have the `TOKEN_FSTRING` go *before* an interpolated part and include that interpolated part, and end the string with a regular string
    - i.e. `"Tea will be ready in ${steep + cool} minutes"` would be:
        - `TOKEN_FSTRING "Tea will be ready in`
        - `TOKEN_IDENTIFER "steep"`
        - `TOKEN_PLUS "+"`  
        - `TOKEN_IDENTIFIER "cool"`
        - `TOKEN_STRING "minutes"`
