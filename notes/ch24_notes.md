# Ch. 24: Calls and Functions

- this whole chapter is devoted to functions

## Function objects

- The stack in the VM is already for local variables and temporaries,
but we have no notion of a *call* stack
- from th VM's perspective, what is a function?
    - it's just some bytecode
    - we could compile the whole program into one chunk and have a pointer to the first
    instruction of its code inside of it
        - this is how native code/assembly works...and how I thought we'd handle it
- however, Nystrom believes it better to give each function its own Chunk

## Compiling to function objects

- compiler assumes it is always compiling to one single chunk
    - with each function's code in different chunks...things get more complicated
- compiler reaching a function declaration must emit code into the function's chunk
when compiling it
    - then when it's done it has to go back to the previous chunk
- fine for code inside function bodies...but what about "top level" code?
    - can simplift compiler by placing the top-level code inside an automatically defined function
        - like an implicit `main`
    - except global vars have different scoping rules than local vars
- we create an ObjFunction *in the compiler*, even though that's the *runtime representation*
of a function
    - functions are similar to literals
    - function *declarations* truly are literals
    - further, all the data for a function is known at compile time
        - ...except for closures...

## Call Frames

- two main problems to handle for the VM:

### Allocating local variables

- the compiler allocates stack slots for locals
    - how does that work when locals are distributed across multiple functions?
- option 1: keep them separate. Each function gets its own dedicated set of slots in the VM
which it would own even when the function wasn't called
    - some early languages did this
    - really inefficient
    - recursion?
- value stack relies on the fact that local vars and temporaries behave as LIFO (stack) fashion
    - still true with function calls.
- consider the following:
```
fun first() {
    var a = 1;
    second();
    var b = 2;
}  

fun second() {
    var c = 3;
    var d = 4;
}  

first();
```
A step through the program looks like this:
    - call `first()`
        - allocate `a = 1`
        - call `second()`
            - allocate `c = 3`
            - allocate `d = 4`
            - return, i.e. dealloc `d, c`
        - allocate `b = 2`
        - return, i.e. dealloc `b, a`
    - empty stack!
- with this property allocating locals should be possible on the VM's value stack
- we want (but is not possible) to determine *where* on the stack each variable will be
at compile time
```
fun first() {
    var a = 1;
    second();
    var b = 2;
    second();
}  

fun second() {
    var c = 3;
    var d = 4;
} 

first();
```
For this series of calls,
    - the first call to `second` would need `c, d` to go in slots 1 and 2
    - but in the second call, `b` needs to be allocated, so `c, d` need to go in slots `2, 3`
    - thus, the compiler can't know exactly where each local variable is across function calls
- however, *within* a function, the *relative* location of local variable is fixed
    - i.e. variable `d` must always be just to the right of `c`
- when a function is called, we don't know where the top of the stack will be
    - but where the top happens to be, we know where all the local variables will be
    - how is this different than a stack frame?
- at the beginning of each function call, the VM records the location of the first slot
where that function's own locals begins
    - instructons to access locals are then relative to that position
    - these relative positions are given at compile time as bytecode
    - the absolute index is calculated at runtime by adding the function call's starting slot
- this recorded location is known as the **frame pointer** or **base pointer**

### Return addresses

- when we call a function we just set `ip` to the first instruction in that chunk
    - how to return to the outer function?
- for each function call we need to track *where* we jump to (the jump register!)
    - called the **Return address**
    - property of *invocation* because of recursion implying multiple return addresses
> As a note, Fortran didn't support recursion. So any function needed only a single return address.
> Thus, the program would *modify its own code* to change a jump instruction to its caller
at runtime.

### Call stack

- so we also store the where the caller should resume when it's done with the callee
- createa an array of CallFrame structs up front as a stack for the VM
- each CallFrame has its own `ip` now and pointer to the ObjFunction
- robust implementation would do better to guard against the stack overflowing


## Function declarations

- function declaration just creates and stores one in a new variable
- parse the name just like any other variable declaration
    - inside a function it's a local variable
- variables got defined in two stages so you can't access its value in its initializer...
but this is allowed for functions because of recursion
    - the function can't actually be *called* and executed until after it's fully defined

### A stack of compilers

- Compiler struct stores data about local variables and scoped nesting
    - however, the front end needs to handle compiling multiple functions nested in each other
- we give each function a separate Compiler
    - starting to compile a function declaration puts another Compiler on the C stack
    - `initCompiler` sets that as the `current` compiler
    - emits bytecode for that function's chunk
    - `endCompiler` returns the new function object which is stored as a constant in the surrounding function's table
        - new problem...that's stored in the VM, NOT in the table...
- how do we get back to the surrounding function?
    - overwritten with `initCompiler`, so we store the nested Compiler structs as a stack too 
    - we use a linked list for this stack instead of an array
- no need to dynamically allocate because each Compiler is on the C stack

### Function parameters

- parameter is just a local variable declared in outermost scope of function body
    - can just use existing support to parse and compile parameters
    - no code to initialize (that's done later)
- in creating this function we copy the name string because the original source code may be freed

## Function calls

- the `(` actually acts as an infix operator
    - higher precedence expression on the left (a single identifier, usually)
    - `(` in the middle
    - arguments at the right
- put arguments on the stack

### Building arguments to parameters

- consider this example:
```
fun sum(a, b, c) {
    return a + b + c;
} 

print 4 + sum(5, 6, 7);
```
Pausing the VM on the `OP_CALL` instruction for `sum()` has the stack look like this:
    - `script | 4 | sum | 5 | 6 | 7`
    - the compiler, in compiling `sum()`, allocated slot 0 for the function name
    - allocated local slots for the parameters in order
    - we need to initialize a CallFrame with the function being called and the region of stack slots
    with the parameters
- the argument slots for the caller and the parameter slots for the callee line up very nicely
    - these windows may be overlapped
    - any temporaries above those slopts must have been discarded by now
    > the calling convention for this is from Lua
- there's no work to "bind arguments" or copy values or anything...the arguments are exactly in their proper locations

### Runtime error checking

- this technique of overlapping the stack windows for the arguments and parameters
relies on the user actually calling the proper number of things
    - this is a runtime error in Lox

### Printing stack traces

- simply aborting on a runtime error doesn't help the user that much
- the classic tool to help with this is a **stack trace**
    - print out of each function that was still executing when the program died
- there's debate on the order of stack frames in a trace
    - most put the innermost function first and work their way backwards
    - Python does the opposite

### Returning from functions

- time to actually implement some code for `OP_RETURN`
- note that there's really not much change to implement a return value, it just sits on the stack
    - very different from jlox which required exceptions to unwind the stack if we were deeply recursed
    - the byetecode compiler flattens out any deep recursion from before
- we need to be careful about returning from top-level (not allowed in Lox)
    - feels rather sensible to me though

## Native functions

- the VM backbone is here but can't really do anything but print
- **native functions** are the way a programming language can do things like check time, read user input, etc.
    - Lox is fairly complete at the language level, but it's really a toy language because of a lack of native functions
- native functions are different from Lox functions because they *don't* push a CallFrame
    - so, we make them a different object type entirely
- we create a helper function to define new native functions exposed to Lox programs

## Exercises

1. Reading and writing the `ip` field is very, *very* frequent. 
This field is accessed via a pointer to the CallFrame, so a pointer indirection is needed.
This can slow down the CPU because of cache misses.
Ideally, `ip` would be in a register.
This can't be required without doing assembly inline, but storing `ip` in a C local variable called
`register` can get the C compiler to do so.
This means we have to load and store the local `ip` back into the correct CallFrame when
starting and ending function calls. Implement this optimization.
Does it affect performance?
Is the complexity worth it?
    - It actually made things *slower* when I did it...so I didn't do this.
    - Nystrom's machine saw a performance boost though...
    - maybe Clang is just a way better compiler

2. Native function calls are fast because the number of arguments are not validated.
This isn't the best...arity checking is kind of important.
Add arity checking.
    - see next one

3. There's no way for a native function to signal a runtime error right now.
This is also kind of important, as native functions are called from Lox's dynamic perspective
Extend native funtions to support runtime errors.
    - decided to do this differently than Nystrom as I didn't want to directly modify the Value stack
    - instead passed a pointer to a boolean in each native function
    - if it's false, then there was no error, so the return value is treated as normal
    - if it's true, then there was an error, so the return value is treated as an error message
    - then the caller of the native function (`callValue`) reports the runtime error

4. Add some more native functions to do things you find useful.
Write some programs using those. What did you add?
How do they affect the feel of the language and it's practicality?
    - makes it way more fun to use!
