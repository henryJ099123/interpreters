# Interpreters

Henry Jochaniewicz

Learned from *Crafting Interpreters*

## Description

## Topics Learned

- Tokenizing / lexing
- Parsing
- Abstract syntax trees
- grammars and how to implement them in parser
- Interpreting an abstract syntax tree
- Visitor pattern of OOT
- native functions
- Environments
- Closures

## Language Features

The language (Lox) is a dynamic, object-oriented language.

### Features implemented from the book

- single-line comments (with `//`)
- "number" (int/double), string, and boolean types
- variable declaration with `var`
- variable accessing and mutating
- scopes with closures and environments
- block statements using `{}`
- control flow:
    - loops (`for` and `while`)
    - if statements
- function declarations
- class declarations
- objects and method calls
- inheritance
- `this` and `super` 

### Features that I implemented (exercises or for fun)

Nearly all the exercises I implemented completely by myself.
The only ones where I got a bit of help from Nystrom's books were:
- ternary operator (for grammar precedence, before I realized the C operator precedence just tells you)
- anonymous functions (implemented it fine myself, cleaned up the code with his solution)
- static methods (the metaclass approach he set up was weird...got about 75% of the way myself)
- `break` (I've never used exceptions much yet)

>My own ideas (therefore no help available) will be marked with a (\*) at the end.

- multiline comments with `/* */` (but without nesting, but that'd be easy to implement)
- explicit division by 0 error
- `+=`, `-=`, etc. syntactic sugar (\*) 
    - for anything that can be updated (variables, object fields, sequence indexing)
- `++` and `--` increments (both pre- and post-) (\*) 
- the ternary operator `? :`
    - I explicitly ignored C's precedence levels for this because it didn't allow things like
    ```
    false ? a = 4 : b = 5;
    ```
    without throwing an error (because it would try to set `5` to the ternary expression. (\*)
    - so I followed C++'s precedence order instead
- the comma operator `,` (as in C)
- anonymous functions (think JS or lambdas)
- String with number concatenation
- logical XOR operation (\*)
- `break`
- `continue` (\*)
- an "`aftereach`" to each loop (\*)
    - a loop feature that occurs after each iteration of the loop
    - occurs with `continue` but not with `break`
- error for an unused local variable
- native function `exit` for the REPL (and for the other one, too) (\*)
- static methods for classes via a "metaclass" approach
    - this one I was 75% of the way and used Nystrom's answer to finish
- user input (for a line) (\*)
- file reading (\*)
- lists (\*)
    - instantiated like `[a, b, ..., z]`
    - supports objects of multiple types
    - indexing via `[n]` syntax
    - setting things with that syntax
    - list concatenation with `+`
    - no append...but you can do so via a concatenation
- 


## TODO

- jlox:
    - *implement `++`, for pre-increment*
    - *implement `++`, for post-increment*
    - *implement lists*
        - very proud of this one!
    - implement strings as a sequence
    - implement `for...each` loops (using iterables?)
    - *implement `break`*
    - *implement `continue`*
    - any other possible native functions (such as file I/O?)
        - *`exit`*
        - file I/O
            - writing
            - *reading*
        - *input*
    - *anonymous functions*
        - *fix the code to be cleaner for anonymous functions*
    - *added `+=`, `/=`, `-=`, `\*=`*
    - *added errors for not changing code*
    - *add `+=`, `/=`, `-=`, `\*=` for methods*
        - *add `++`/`--` for methods*
    - *add static methods*
    - add getters and setters
