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

### How to use

You need some C compiler.
The Makefile included in the folder
`c_implementation` uses Clang,
but you can edit the Makefile to replace it with gcc.

Run
```text
make
```
to generate the executable. It will be called `clox`.

Run
```text
./clox [some_file]
```
to run the interpreter on a `.lox` file.
Or, for the REPL:
```text
./clox
```
Enjoy!

### Summary of interpreter

`clox` is an interpreter of the toy dynamic language Lox.

It uses a single-pass compiler to generate bytecode
that is then interpreted by a virtual machine
to execute code.

It is far faster than the `jlox` implementation:
in generating the 40th fibonacci number
(see `tests/fib.lox`), `clox` was nearly twice as fast as
`jlox` on my machine with full optimizations for java.
Without those optimizations, the java implementation
does not even finish in a reasonable amount of time.

The parser is a Pratt parser and thus requires no
syntax tree shenanigans. It outputs bytecode
that modifies the VM's stack to execute instructions.

The dynamic values have two implementations:
one via *tagged unions* and another via *NaN boxing*.
An implementation can be chosen at compilation of the C
program via a flag in the `common.h` file.

All 'Object' types in Lox are treated the same
via *struct inheritance*, where the first element of each
Object subtype is an Obj struct, which allows pointer
casting.

Strings are *interned* in this implementation for fast equality checks.

Global variables *in the book* are stored in a hash table at runtime.
In my implementation, the compiler allocates an index for every global
variable found in an array and passes these as indices to the VM,
making accesses and mutations of global variables far faster.

Local variables remain on the VM stack and are resolved as indices
into the stack by the compiler. If they are part of a closure,
they are "lifted" and pointed to by a heap-allocated *upvalue*.
Each function holds an array of these at runtime.

This implementation has a garbage collector to handle memory allocation.
It uses a *mark-sweep* garbage collector that marks *roots*
and anything referred to by those roots, and cleans memory
at intervals based on the size of the heap after cleaning.
It cleans interns strings only if they aren't pointed to by anything
still live, which means they are *weak references*.

Lox also supports classes and objects. Fields are accessed via a table
on each instance. Methods are accessed via a table on each class.
`this` is a local variable that is instantiated as receiver of the method.
Inheritance is *copy-down*, meaning the subclass has no explicit reference
to the superclass, except via a `super` invocation.

The VM executes via a stack as well to perform computation
and its instructions.

I wrote a few short benchmarks to test differences in execution time.
At full optimization for both `clox` and `jlox`, clox outperforms by
a factor of at least 2.
A hash table optimization improved the speed of a relevant benchmark
by about 33%.
NaN boxing optimization improved the speed of all benchmarks by about 10%.

### Topics learned

Granted, I knew a lot of these already,
so consider (most) of these refreshers.

- pointer and macro magic
- tries (a finite automaton to complete strings for tokens)
- single-pass compilation
- variadic functions
- hash tables with linear probing
- string interning
- bytecode (opcodes, arguments, encodings, etc.)
- tagged unions
- callframes
- native functions
- upvalues (a Lua implementation thing)
- garbage collection
- NaN boxing
- benchmarking, performance, and optimization

### My additions

- line numbers for a piece of bytecode in the book are found via a parallel array
of integers. Line numbers in *my* implementation uses run-length encoding
to get each line number, which saves a lot on space for slightly slower runtime,
but this only applies for debugging or errors, which is *not* the common case.
- The constant table for each chunk of bytecode was capped at 256 values.
I extended this to by dynamically large, which needed modified `_LONG` versions
for almost every opcode the book uses.
- Implemented multiline comments, like `/* */`.
- Implemented `switch` statements. You can have an arbitrary number
of cases because for completing a case, each case has a jump instruction to the next
one that pushes the instruction pointer forward to the end of the statement.
- Implemented runtime errors for native functions. I passed a pointer
of a bool to each native function to see if there was an error. If there was,
the VM unpacks the result as an error message. Otherwise, it treats it as
a regular return value.
- Added the ternary operator `? :` with C++ operator precedence.
- Added `break` and `continue` statements.
- Added some native functions like `sqrt` and `inputLine` (user input).
    - These have runtime errors too!
- Optimized the global variable lookups (following an exercise from the book).
Globals are accessed via their string names in a table at runtime, normally.
I changed it to be accesses to indices of an array at compile time,
so rather than passing the string names as arguments to bytecode for the VM,
the compiler passes the indices to this array, which each global variable
gets a spot for. This speeds up execution a lot.
- Implemented constant variables. Globals are done via a hash table that acts
as a set at compile time. Locals store a field saying whether or not
they are modifiable. In both cases,
the actual VM's execution of variables, setting, and getting is all the same.
Constants are only recognized at compile time.

### TODO

- implement my own version of `malloc`
- *implement faster line number implementation using run-length encoding*
- *implement `OP_CONSTANT_LONG`*
- *implement `/* */`*
- *implement the `switch` statement*
- *runtime errors for native functions*
- *consts*
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
- fix the `makeConstant` return value
- fix the memory leak in `identifierConstant`
- optimize that I do a reverse search on the variables for the debugging. I could just do another list
- *fix an error I think with the `_LONG` varieties and the now-used method for global variables*
- allow more than 256 locally scoped variables at a time
- do exercise 23.4
- add more native functions
- `try...except`
