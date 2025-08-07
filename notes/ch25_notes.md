# Ch. 25: Closures

- consider the following example:
```
var x = "global";
fun outer() {
    var x = "outer";
    fun inner() {
        print x;
    } 
    inner();
}
outer();
```
This should print `"outer"`. It instead prints `"global"`.
    - must include the entire lexical scope of all surrounding functions when resolving a variable
- harder in clox because the bytecode VM stores locals on a stack
    - used a stack because locals supposedly had stack semantics...but with closures, this is only half-true
```
fun makeClosure() {
    var local = "local";
    fun closure() {
        print local;
    }
    return closure;
} 

var closure = makeClosure();
closure();
```
The function `makeClosure()` declares the local variable `local` and a function `closure`.
    - `closure` uses the variable `local` and thus must "capture" it
    - Then `makeClosure` returns a reference to `closure`
    - thus, `local` must outlive the function `makeClosure`
- this could be solved by dynamically allocating memory for all local variables
    - this is jlox's approach via the Environment objects
    - however, the stack is *really* fast and therefore better
    - most local variables are *not* captured by closures, so it isn't worth sacrificing the common case here
- will result in two different implementation strategies:
    - regular locals will stay as currently done
    - locals captured by closures will "lift" them onto the heap
> Closures have been around since Lisp, see "closure conversion" or "lambda lifting" for some optimization techniques
- Nystrom is adopting the technique of the Lua VM

## Closure objects

- there is no operation to "create" a function at runtime...they're constants instantiated at compile time
    - functions are a kind of literal, a pure piece of syntax
    - all of the data is known at compile time
    - this is *no longer the case* with closures
- we need a runtime component for the closure that captures the local variables surrounding the function as they exist at execution
- the ObjFunction type represents the "raw" compile-time state of a function declaration
    - all closures created from a single declaration share the same code and constants
    - at runtime, we wrap this ObjFunction in an ObjClosure structure
    - this new struct has a reference to the bare function along with runtime state for the locals
- we will wrap *every* function in an ObjClosure for simplicity

### Compiling to closure objects

- we need the compiler to emit instructions to tell the runtime when to make a new closure

### Interpreting function declarations

- need to change everything that used ObjFunction to now use ObjClosure

## Upvalues

- existing instructions for reading/writing locals are limited to a single function's stack window
    - locals from surrounding functions are outside of scope
    - need to be able to look there
- easiest approach: instruction that takes relative stack slot offset that reaches *before* current function window
    - would work, if closed-over variables always on the stack...but these variables need
    to outlive the functions where they are declared
- okay, so maybe we take every local variable that gets closed over and have it live on the heap
    - would work fine if clox didn't have a single-pass compiler
    ```
    fun outer() {
        var x = 1;
        x = 2;
        fun inner() {
            print x;
        } 
        inner();
    } 
    ```
    In this example, `x` is compiled and emits code for assignment afterwards.
    This is performed *before* the declaration of ` inner`, in which we know `x` is closed over.
    - i.e. we do not have an easy way to fix the already emitted code to treat `x` differently
    - we want a solution that allows a closed-over variable to live on the stack normally
    *until it is closed over*
- Nystrom takes a page from Lua using something called an **upvalue**
    - it refers to a local in an enclosing function
    - every closure maintains an array of these upvalues, one for each surrounding local variable
    used by the closure
    - the upvalue has a pointer back into the stack where the variable it captured lives
        - when the closure needs to access this variable, it goes through the upvalue to reach it
        - the VM creates an array of these upvalues and wires them to "capture" those locals

### Compiling upvalues

- we have enough information *at compile time* to resolve the locals a function accesses outside itself
    - so...we know *how many* upvalues a closure needs and *which* variables they capture
    **AND** which stack slots they are in

### Flattening upvalues

- closure accesses a variable declared in the immediately enclosing function,
but they can be in any enclosing function, as seen here:
```
fun outer() {
    var x = 1;
    fun middle() {
        fun inner() {
            print x;
        } 
    } 
} 
```
`inner` takes a page from `outer`, not from `middle`.
- this seems like it'll be fine, it will just be farther down the stack, but consider this:
```
fun outer() {
    var x = "value";
    fun middle() {
        fun inner() {
            print x;
        } 

        print "create inner closure";
        return inner;
    } 

    print "return from outer";
    return middle;
} 

var mid = outer();
var in = mid();
in();
```
When ran, this should print:
```
return from outer
create inner closure
value
```
But importantly, `outer()` returns and pops all of its variables off the stack
before the *declaration* of `inner` executes
    - `x` is already off the stack by the time the closure for `inner` is made
    - in the true execution flow of this program,
        - `outer` returns and pops all of its variables off the stack
        before the *declaration* of `inner` executes in the VM
        - then when the closure is made for `inner`, `x` is already gone!
- we have two problems:
    - we need to resolve local variables that are declared in surrounding functions
    beyond the immediately enclosing one
    - we need to capture variables that have left the stack
- upvalues solve the latter problem, so we can let them do the former too
    - we let a closure capture either a *local* or an *existing upvalue*
    in the immediately enclosing function
    - if a deeply nested function references a local variable declared several
    nestings outside of the nested function,
    we can thread them by having each function capture an upvalue for the
    next function to grab
- so in that prior example, `middle` captures `x` in the immediately
enclosing function `outer` and stores it in its own upvalue, even though it doesn't
actually reference `x`
    - then `inner` grabs the *upvalue* from the ObjClosure for `middle` that captured `x`
    - a function captures *only* from the immediately surrounding function
        - *that* function is guaranteed to still exist at the point of inner function declaration
- the `OP_CLOSURE` instruction has variably sized encoding
    - for an upvalue, there are two single-byte operands
    - if the first byte is 1, it captures a local variable in the enclosing function
    - if the first byte is 0, it capturs an upvalue
    - the next byte is *either* the local slot *or* the upvalue index to capture
- so, in summary, the compiler outputs an `OP_CLOSURE` instruction
followed by a series of operand byte pairs for each upvalue it needs to capture
at runtime

## Upvalue objects

- each `OP_CLOSURE` instruction has a series of bytes that specif the upvalues
that the ObjClosure should own, but these need a runtime representation
- upvalues must manage closed-over variables that no longer live on the stack
    - i.e., dynamic memory allocation, so the Object system
- note that the ObjUpvalue has a *pointer* to a Value, so it references
a *variable*, **not** a value
    - assigning the variable to the upvalue means we're assigning the variable,
    not a copy
    ```
    fun outer() {
        var x = "before";
        fun inner() {
            x = "assigned";
        } 
        inner();
        print x;
    } 
    outer();
    ```
    This ought to print "assigned", not "before".

### Upvalues in closures

- just some vm support

## Closed upvalues

- this only works for examples where the local is still on the stack...
    - the issue is that variables in closures *don't have stack semantics*

### values and variables

- does a closure contain a *value* or a *variable*?
```
var globalSet;
var globalGet;

fun main() {
    a = "initial";
    fun set() { a = "updated"; }
    fun get() { print a; }

    globalSet = set;
    globalGet = get;
} 

main();
globalSet();
globalGet();
```
`main` here creates two closures and stores them in global variables.
    - both of these closures capture the same variable `a`
    - the first closure *assigns* a value to it
    - the second closure *reads* the value
- so...what does `globalGet()` print?
    - if closures close over values, then each closure gets its own copy of `a`
    with the value of `a` at the point of function declaration
        - in this case, it prints "initial"
    - if closures close over variables, then `get` and `set` reference the same
    *mutable variable*
        - there is only one `a`
        - thus, it prints "updated"
- Lox, and most languages with closures, is the latter: closures capture variables
    - they capture *the place the value lives*

### Closing upvalues

- local variables always start out on the stack
    - closed over variables move to the heap
- we'll use **open upvalue** to refer to an upvalue that points to a local still on the stack
- use **closed upvalue** for one that moves to the heap
    - where on the heap does this variable go?
        - the ObjUpvalue for it, in a new field
    - when do we close the upvalue?
        - as late as possible, right when we go out of scope
- `endScope` already emits `OP_POP` for each local variable that goes out of scope
    - if the variable is captured by a closure we instead emit a different instruction
    - so, we must store which locals are closed over
- compiler already has an array of Upvalue structs for each local variable in the function
that tracks this state
    - this is good for knowing which variables a closure closed over,
    but not good for knowing if a variable is closed over
    - i.e. the compiler maintains pointers from upvalues to the locals they capture,
    but not in the other direction
    - so, we need to add extra tracking inside the Local struct

### Tracking open upvalues

- if multiple closures access the same variable, they should reference the exact same storage location
    - right now, the VM creates a separate Upvalue for each one...which is no good
        - for the non-nested case
- whenever the VM needs an upvalue that captures a local slot, first search for an existing upvalue
    - we reuse that if found
    - all previously created upvalues are hidden in various closures and the upvalue arrays
- need to give the VM its own list of all open upvalues that point to variables on the stack
    - searching a list for an upvalue sounds slow but not that bad
        - closure *invocations* need to be fast, not *declarations*
    - can order the list of open upvalues by the stack slot index they point to
        - common case: slot has *not* been captured
        - means we want a linked list, not a dynamic array
- on our loop traversal of upvalues in this linked list, we can exit the loop in 3 cases:
    - we found the upvalue in the local slot, so we reuse it
    - we ran out of upvalues to search
    - we found an upvalue below the one we're looking for

### Closing upvalues at runtime

- remember that the compiler emit an instruction to tell the VM when a local var
should be hoisted onto the heap
- when the compiler reaches the end of a block, it discards all locals
    - it emits `OP_CLOSE_UPVALUE` for each local closed over
    - the compiler does not emit any instructions at the end of the outermost
    block scope that defines a function body
    - that scope contains the parameters and locals declared immediately inside the function
    - those need to get closed too
        - we need extra steps here because we don't pop those values since the runtime
        implicitly pops them by popping the call frame and adjusting the stack pointer
- holy moley it's done
    - way easier in jlox because closures fell out naturally from environments
    - in clox, the VM treats variables in closures differently
- in complexity, jlox's closures were free, butin *performance*, there was a significant price
    - allocating all environments on the heap
- users do not perceive this complexity, i.e. it looks simple from the surface
    - *optimization* tends to be detecting certain uses and providing faster path for code for that
- next step: garbage collection

## Exercises 

1. Wrapping every ObjFunction in an ObjClosure results in a level of indirection
that has a performance cost, which isn't necessary for functions that have no
closed over variables.

Change clox to only wrap functions in ObjClosures that need upvalues.
How does the complexity and performance change?

2. How *should* Lox behave regarding closing over loop variables?
Change the implementation to create a new variable for each loop iteration.

3. "Objects are a poor man's closure." Using closures,
write a Lox program that models two-dimensional vector "objects."
    - it should define a "constructor" function to create a new vector
    with given `x` and `y` coordinates.
    - provide "methods" to access the `x` and `y` coordinates of values returned from that.
    - define an additional "method" to add two vectors and produce a third.


## Design note: Closing over the loop varaible

- recall that closures capture variables: when two closures capture the same variable,
they share a reference to the same underlying storage location.
- Consider this example;
```
var global1;
var global2;

fun main() {
    for(var a = 1; a <= 2; a = a + 1) {
        fun closure() {
            print a;
        } 
        if(global1 == nil)
            global1 = closure;
        else
            global2 = closure;
    } 
} 

main();
global1();
global2();
```
Here, `main` does two iterations of a `for` loop.
Each time in the loop it creates a closure that captures the loop variable.
    - the first closure goes in `global1` and the second in `global2`
- now the question is, did it close over two different variables?
    - is there only one `a`, or did each iteration get its own?
- typically, if a language has a higher-tier iterator-based looping structure,
a. la. `foreach`, then it makes sense for a new variable on each iteration
    - the code *looks* like a new variable, and there's no increment expression
    - this is what users expect, too
- however, it's almost *never* useful for each iteration to share a loop varaible
    - this is only even detectable in closures
- make it *look* like a mutation, but create a new variable each time
