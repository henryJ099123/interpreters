# Ch. 21: Local Variables

- the approach to globals is different than for locals
    - globals are *late-bound*, i.e. resolved after compile time
- if locals are slow, *everything* is slow, so locals are resolved at compile time
- locals are lexically scoped, so can resolve just via the text of the program

## Representing local variables

- C and Java manage the locals via the stack
    - via native stack mechanisms
- currently the stack is only used for *temporaries*, short-lived data segments for expression computation
    - local variables can live on the stack, too, as long as they don't interfere with these
    - can index down if we know where the thing is
    - VM still expects a stack to act like a stack though
- new locals are always made via declaration statements and statements do not nest in expressions
    - thus, there are *no temporaries on the stack* when a statement begins executing
    - further, blocks are strictly nested
        - innermost block ends $\Rightarrow$ takes the "topmost" declared vars with it
- beyond the stack being used well, we know exactly *where* these variables will be on the stack
    - compiler knows exactly which vars are in scope at a given point, and can thus simulate the stack
- using stack offsets as operands for bytecode instructions that read/store local vars
- requires a lot of state in the compiler

## Block statement

- need local scopes to have local variables
- scope controlled via an integer

## Declaring local variables

- compiler already supports parsing and var declarations
    - just assumes all variables are global
- the flow of logic:
    - begins in `varDeclaration`
    - `parseVariable` consumes the identifier token and adds *used to* add its lexeme to the constant table
        - *now* it adds it to another hash map and returns the index it points to in a separate array
    - then, `defineVariable` emits the bytecode for storing the variable's value in that separate array
        - used to be the globals hash table
- there's *no code* to create a local variable at runtime
    - already executed the code for the variable's initializer
    - value sitting on top of the stack as a temporary
    - new locals are allocated at the top of the stack...so this just becomes the new local value
    - budabing budaboom done! Just need to record that is indeed the variable somewhere
- *declaring* a variable helps the compiler remember it exists
    - if worried about string lifetime for the var's name, the Local directly stores a copy of the Token struct
    - Tokens store a pointer to the first character of their lexeme and the lexeme's length, and thus to the source code
    - thus, no problem here
- must account for invalid code too!
- consider:
```
{
    var a = "first";
    var a = "second";
} 
```
This is allowed at the top level (for the REPL) but in this case this is likely a bug.
    - this is ok if `a` and `a` are in *different* scopes

## Using locals

- can compile and execute local variable declarations, and at runtime tehy're on the stack
- the locals array has the *exact* layout that the VM's stack will have at runtime

### Interpreting local variables

- we push the value of the variable *again* if it's local
    - may see mredundant, but other bytecode instructions just look at the *top* of the stack
    - this helps it remain *stack*-based, which was the intention
    - helps for keeping instruction stuff down
- for the debugging, the local variable's name never leaves the compiler
    - thus we can only see the slot number
    - erasing local variable names would really be a pain if we did this

### Another scope edge case

- variable scoping has a lot of errors
- remember this?
```
{
    var a = "outer";
    {
        var a = a;
    } 
} 
```
We fixed this by splitting variable declaration into two phases.
    - when a variable is initially declared, it is "uninitialized"
    - if it is used before initialization, or before the end of the statement, that's an error
    - we use a depth of `-1` to indicate this special state
- thus, *declaring* is adding variable to scope, and *defining* is available for use
- almost all of the code from this chapter was in the compiler
    - the runtime just has two instructions
    - a trend: pull work forward in the compiler to make the runtime faster
- static types would be an extreme example of this
    - all type checking and error handling in compilation
    - runtime doesn't have ro do anything
    - in C, the type isn't even known at runtime; the compiler erases any representation because it's confident in it

## Exercises

1. 
