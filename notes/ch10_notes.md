# Ch. 10: Functions

## Function Calls

- what does a function call look like?
```
average(1, 2);
```
- the name of the function (e.g. `average`) is not a part of the call syntax
    - the **callee** can be anything that evaluates to a function, e.g. `getFunction()()`
- the `()` is a *postfix operator*
- even higher precedence than the unary operator, so it's near the bottom of the grammar
> `fn()()()` isn't common notation in C, but it is in languages like ML. This is called **currying**.
> we don't want arguments to be the comma operator, because that's not what that comma is intending to do, so we have to have the `argument` rule go back not all the way to `expression` but skip `comma`
- this is cumbersome but the norm :(
- the syntax tree node holds the callee expression and a list of expressions for the argument, and a token of the closing parenthesis for error handling

### Max number of arguments

- Should we limit the number of arguments to a function?
    - C standard argues there should be at least 127 arguments
    - Java specifies a method can have no more than 255 arguments
    - a max number of arguments will help the bytecode interpreter, so we go with max of 255
- remember, *throw* an error means the parser is in panic mode
    - *reporting* an error means the parser still knows kind of what to do

### Interpreting function calls

- keep in mind that evaluating arguments may have side-effects
    - neither C nor Scheme specifies the order in which arguments must be evaluated
- building a LoxCallable interface which any function will implement, and class objects
    - anything that looks like it is called

### Call type errors

- not everything is callable!

### Checking arity

- need to make sure the number of arguments is consistent
- different languages handle this differently
    - some make it a compile time error
    - some fill in what's missing
    - Python throws a runtime error (as of course it does)
- Lox will do a runtime error

## Native functions

- functions the interpreter exposes to the user but are implemented in the *host language* instead of the *implementation language* are called **native functions**
    - a.k.a. **primitives**, **external functions**, or **foreign functions**
    - these native functions are key to providing access to very imoprtant fundamental services, like I/O
    - e.g. `print` is usually a native function
- users can also provide their own native functions sometimes, called **foreign function interface** or a **native interface**

### telling time

- **benchmarks** important so I'm assuming we're making a `clock` function
    - I was right
- recall the `environment` field in the interpreter changes from scope to scope
    - so we have a static `globals` field to hold a reference to the global environment
- some languages separate the variables from the functions but that's a bit silly methinks for right now
    - **Lisp-1**: languages that group functions and variables together in same namespace
    - **Lisp-2**: languages that don't

## Function Declarations

- function declarations bind a new name, so they are *only allowed* where *declarations* are allowed
    - a named function declaration is *syntactic sugar* for *creating a new function object* and *binding a new variable to it*
    - these wouldn't be needed if there were anonymous functions, i.e.
```
var add = fun(a,b) {print a + b;};
```
- made a helper rule for the main declaration rule for classes and similar things
    - body of a function is FORCED to be a block
    - its arguments must be a list of IDENTIFIERS

## Function Objects

- what is a Lox function in our Java code?
    - going to be an object, obviously
    - track parameters and code body
    - but we also need to *implement* the LoxCallable so we can actually call the function
    - do not want the runtime phase of the interpreter to bleed into the syntax classes, so `Stmt.Function` won't directly implement that; instead, we have a new wrapper class
- the `call` function of the new class `LoxFunction`:
    - it takes the interpreter (for what it does) and the arguments
    - it *adds* the arguments as *values* to the parameters as new *variables* to the *environment* of the interpreter
    - the environment doesn't care about the current local stuff, just global variables, when referring to outside functions
    - further, those outside arguments aren't needed when the function is done being called
    - the *one thing* I am not sure about is the `return null;` which I feel should return something if there's a `return` statement, but we haven't gotten there yet
- functions **encapsulate** its parameters (i.e. it's a *separate environment*); no other code can see it
    - so a dynamic environment where each function *call* gets its own environment
    - if each *function* got its own environment, there'd be an issue for a recursive functions
    - this differing environment business means the same call to the same function with the same code may produce different results (bc of side effects)
    - `executeBlock` undoes the environment change
    - this *code* now becomes an **invocation**
- assumption that the argument list and the parameter list is the same, which is OK bc the `visitCallExpr()` checks the function's `arity` before calling `call()`

### Interpreting function declarations

- interpreter takes the *syntax* of the function and converts it to a runtime representation of it
    - function declarations also *binds* the resulting object to a new variable

## Return statements

- no way to get results back out now
    - Lox's statements don't produce values, so need dedicated statement which does
    - return value optional to support early exitin
- as Lox is dynamically typed, there are no `void` functions (every function returns *something*)
    - return `nil`

### Returning from calls

- interpreting these are tricky
    - return from anywhere in a function
    - interpreter needs to jump out of whatever context it is in
- solution: exception
    - use exception to unwind an interpreter past its visit methods and back to `call()`
    - not entirely popular but definitely the best way for a recursive tree-walk interpreter
- this works!

## Local Functions and Closures

- small problem: when `call()` creates a new environment...who's its parent?
    - it's `globals`, the highest level environment,
    which is fine for top-level defined functions
- but function declarations are allowed *anywhere* a name can be bound
    - i.e. inside of blocks or other functions
    - **local functions** (like Python)
- consider the following:
```
fun makeCounter() {
    var i = 0;
    fun count() {
        i = i + 1;
        print i;
    }
    return count;
}

var counter = makeCounter();
counter(); // prints 1
counter(); // prints 2
```
- this is weird:
    - `count()` uses `i`, which is declared in the containing function `makeCounter()`
    - `makeCounter()` returns a reference to the `count()` function and then it's done
    - the top-level invokes `count()`, which assigns to and reads `i`, even though the function *declaring* it is now done (`makeCounter`)
- this will can an error now because `count()` expects `i`, which only exists in `makeCounter()`
    - so, we need to *bind an environment* to each function
- this is called a **closure**, which *closes over* and holds onto the surrounding variables when the function is declared
