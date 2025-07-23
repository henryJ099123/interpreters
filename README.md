# Learning about interpreters

Henry Jochaniewicz

Learned from *Crafting Interpreters*

## Description

Here's my work, additions, and notes from reading through Robert Nystrom's
book [*Crafting Interpreters*](https://craftinginterpreters.com). The book involves implementing a simple but functional
language "Lox" that is dynamically typed and object-oriented (see below).
The textbook is split into two parts:
an implementation using abstract syntax trees in Java and another implementation
using bytecode compilation in C.

At least for the Java section, I expanded on the implementation in the book quite a bit,
to the point where it's basically a functioning high-level (ish) language.
For actual use I would need to add a hashmap and error handling (like `try` and `except`),
but I am confident I could do so...I just wanted to move onto the C part.

## `jlox` (Java implementation)

I do not feel like writing a style guide so just look at Nystrom's book for language features.
See [below](#my-additions) for my additions to the language.

### How to use

You need Java on your computer.

Run the following:
```
./make.sh
```
If this doesn't work, make sure it is executable:
```
chmod +x ./make.sh
```
Then to run the interpreter (if in the main directory):
```
./jlox [some_file]
```
There's a host of practice files in `practice_files/`.
Or, for the REPL:
```
./jlox
```
Enjoy!

### Topics Learned

- getting better at Java
- Tokenizing / lexing
- Parsing
- Abstract syntax trees
- grammars and how to implement them in parser
- Interpreting an abstract syntax tree
- Visitor pattern of OOT
- native functions
- Environments
- Closures

### Language Features

The language (Lox) is a dynamic, object-oriented language.

#### Features implemented from the book

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
- native function `clock`
- class declarations
- objects and method calls
- inheritance
- `this` and `super` 

#### My additions

Nearly all the exercises I implemented completely by myself.
The only ones where I got a bit of help from Nystrom's books were:
- ternary operator (for grammar precedence, before I realized the C operator precedence just tells you)
- anonymous functions (implemented it fine myself, cleaned up the code with his solution)
- static methods (the metaclass approach he set up was weird...got about 75% of the way myself)
- `break` (I was being stupid)

>My own ideas, i.e. the not-exercises, will be marked with a (\*) at the end.

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
    ```
    while(condition) {
        ...
    } aftereach {
        ...
    }
    ```
    - a loop feature that occurs after each iteration of the loop
    - occurs with `continue` but not with `break`
    - Is this a dumb addition? Yes. Is it kind of fun? A little!
- error for an unused local variable
- native functions (\*):
    - `exit()` for the REPL (and for the other one, too)
    - `length(sequence)` to get length of string or list
    - `openForRead(filepath)` to open file for reading
    - `readLine(f)` to read line from file opened for reading
    - `openForWrite(filepath)`
    - `openForAppend(filepath)`
    - `write(f, message)`
    - `close(f)`
    - `inputLine()` to get user input (for a line)
- static methods for classes via a "metaclass" approach
    - this one I was 75% of the way and used Nystrom's answer to finish
- lists (\*)
    - instantiated like `[a, b, ..., z]`
    - supports objects of multiple types
    - indexing via `[n]` syntax
    - setting things with that syntax
    - list concatenation with `+`
    - no append...but you can do so via a concatenation
- Strings as sequences with indexing as above (\*)
    - made strings immutable for simplicity (hard to check for valid input and throw a catchable error)
        - definitely doable (as in I know how) but I thought not
        - also because I was considering adding a hashmap (would be pretty easy) and wanted immutable things to do so
- forall loops (as in Python's `for i in range(...)` or Java's `for(type t: List<Type> thing)`) (\*)
    ```
    forall(var name: sequence) {
        ...
    } 
    ```


### TODO

Anything italicized has been implemented

- jlox: **DONE** with what I want to do at this moment
    - *implement `++`, for pre-increment*
    - *implement `++`, for post-increment*
    - *implement lists*
        - very proud of this one!
    - *implement strings as a sequence*
    - *implement `for...each` loops (using iterables?)*
        - "hardcoded" with sequences because I don't have any more iterables
    - *implement `break`*
    - *implement `continue`*
    - any other possible native functions (such as file I/O?)
        - *`exit`*
        - file I/O
            - *writing*
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

## clox

### TODO

- implement my own version of `malloc`
- *implement faster line number implementation using run-length encoding*
- *implement `OP_CONSTANT_LONG`*
- *implement `/* */`*
- implement string interpolation with `${...}` syntax
- fix the weird negation bug thing, see 17.4.3 comment
- add the `?:` support
    - add the syntax compiler for it
    - *add actual VM support*
- lists
- indexing
- the modulus `%`
- syntactic sugars:
    - pre- and post-increment and decrement
    - `+=` and the like
- `break`
- `continue`
- file input and output
- user input
- static methods
- `exit` for the REPL
- `forall` loops
- the `aftereach` I had implemented for jlox
- anonymous functions `fun () {...}`
