# Ch. 11: Resolving and Binding

- closures have screwed up the scoping of the interpreter
    - some weird correctness issues now
- goal: *semantic analysis*

## Static Scope

- Lox uses *lexical* scoping
    - i.e. you can figure out which declaration a variable name referes to via the program's texts
```
var a = "outer";
{
    var a = "inner";
    print a;
}
```
- the `a` being printed in this block is not the global one, obviously
    - running the program *cannot* affect this
- **variable usage**: a **variable usage** refers to the *preceding declaration* with the same name in *the innermost scope* that *encloses the expression where the variable is used*
    - "variable usage" to cover both expressions and statements
    - "preceding" means *in program text*
        - preceding in *text* does not necessarily mean preceding in *time*, because of *dynamic temporal excecution* when a function defers a chunk of code
- this rule does not mention anything about runtime, so a variable expression always refers to the same declaration regardless of where in the program we are executing
    - but consider the following:
```
var a = "global";
{
    fun showA() {
        print a;
    }

    showA();

    var a = "block";
    showA();
}
```
- my brain thought that this *should* print `"global"` because a new `a` is made
    - but it *shouldn't* do that, based on closures of other languages, apparently
        - a variable expression ought to always *resolve to the same variable*
- this example never reassigns a variable, and it has a single `print`, but that can be different

### Scopes and mutable environments

- environments are the dynamic manifestation of static scopes
    - the two are *mostly* in sync with each other (new environment whene enter a new scope, remove when leaving the scope
    - one other operation on environments: binding a variable in one
- walk through the above example:
    - `a` is declared in global scope
    - entering block creates a new environment
    - `showA` declared in this environment, which is bound to the new environment
        - this has its own `closure` field that captures the block's environment and holds a reference to it
    - calling `showA()` dynamically creates a new environment for that function body, which is empty as it has no params
        - parent of this environment is the block's environment
    - the `print a;` call looks for the value of `a` by walking up the chain of environments
        - walks up to global environment to do so
    - declare the second `a` inside the block's environment
    - `showA()` called this time finds an `a` in the block environment
- intuition: all the code within a block is in the same scope
    - this is not true, necessarily
```
{
    var a;
    // 1.
    var b;
    // 2.
}
```
- if "scope" is defined as a set of declarations, the scope at line marked 1 is *not* the same as that marked 2, because `b` has not been declared yet
    - it is as if each `var` splits the block into separate scopes
    > Some languages make this explicit, for example Scheme or ML, wherea local variable delineates the subsequent code where the new variable is in scope
- our environments act as if the entire block is one scope that changes over time
    - this is bad for closures, where the function wants to capture a *frozen* snapshot of the environment

### Persistent environments

- **persistent data structures**: a data structure that cannot be directly modified
    - any "modification" produces a brand new object that contains all the original data and the new one
    - imagine tuples in Python
    - sounds like a waste of memory and time, but most data between persistent data structures are shared among their copies
- i.e. every time we declared a variable it would return a *new* environment that contained all the preivous data and the new one
    - creates an implicit split
- this is a common way of solving this issue (Scheme interpreters do this, for example) but this requires changing everything

## Semantic Analysis

- interpreter **resolves** a avariable each time the variable expression is evaluated
    - static scope: variable resolves to the same thing each time by looking at the program
    - so...why is this a dynamic process?
- we only need to resolve each variable *once*
    - i.e. write code that inspects the program, finds each variable, figures out which declaration each refers to
    - called **semantic analysis**
- *parser* tells if code is syntactically correct, semantic analysis tells what code actually means
- recall the environment chaining and walking up caused the issue
    - as long as we walk the same *number* of links in the chain, we would have found the same variable (i.e. the correct scope)
    - when do we do this calculation? It should be in the parser as it is a static property
    - but we'll make it as a separate pass

### A variable resolution pass

- single pass between parser and interpreter to resolve all of the variables
    - if we were doing static types, this would be the time to check that as well
    - any work not reliant on state available at runtime can be done in this way
- static analysis has two important properties:
    - **no side effects**
    - **no control flow**

## A resolver class

- resolving variables is relevant for:
    - block statements (new scope)
    - function declaration (new scope for body)
    - variable declaration
    - variable and assignment expressions

### Resolving blocks

- scopes act as a stack, so use a stack to hold them
    - meant to hold local (block) scopes
- if variable isn't in the stack of local scopes, it is assumed to be global

### Resolving variable declarations

- consider the following example:
```
var a = "outer";
{
    var a = a;
}
```
- what happens here?
    - Option 1: **run initializer, then put new var in scope**
        - new local `a` would be initialized with "outer"
        - so, the declaration `var a = a;` would turn into
        ```
        var temp = a;
        var a;
        a = temp;
        ```
    - Option 2: **put var in scope, then run the initializer**
        - observe var before it's initialized
        ```
        var a; // nil
        a = a;
        ```
        - why is this any good? Will always be `nil`
    - Option 3: **error**
- the first two don't look like what's actually desired, so we'll make this an error, according to Nystrom
    - I actually think it should be the first; I'd expect `a` to have `"outer"` here in the block
- to make this an error, we need to know if we're inside the initializer of a variable

### Resolving variable expressions

- variable declarations write to scope maps
    - maps are *read* in variable expressions

### Resolving...the rest of the stuff

- functions:
    - bind both names and introduce a scope
    - name of function is bound to the surrounding scope
        - bind it sparameters into that inner function scope
    - declare and define name of function in the current scope, we define the name early
        - this lets a function recursively refer to itself
    - create a a new scope for the body, then bind variables for the parameters
        - then it resolves the function body
        - different than a dynamic analysis, when the function body isn't touched until call
        - in a static analysis, the body is immediately analyzed when the function is made

## Interpreting Resolved Variables

- each time the resolver visits a variable, it tells the interpreter how many scopes are between the current scope and the scope of the variable's definition
    - that's what happens in `resolveLocal()`
- this corresponds to the number of *environments* between the current environment and the enclosing one where the variable's value is
    - resolver passes off this information via `Interpreter.resolve()` method
- the question is, where do we store this?
    - many languages store it in the syntax tree node itself
    - another common approach: store it off to the side in a *map* that associates each syntax tree node with its resolved data
    - may be called a **side table**
    > IMO the first approach is way better. I feel we're only doing the second because Nystrom doesn't want to mess with the existing structure of the program.
    - a benefit of storing data in the map is that we can discard the map when needed
        - OK I concede...this is a good use-case
        - also good for IDEs who want to recompile without going thru the whole syntax tree
    - no problem with the map either because each Java object has a unique address

### Accessing a resolved variable

- interpreter now has access to each variable's *resolved* location
    - so any time we need to get a *variable* out, we can just find it in the corresponding syntax tree via the pointer in the map!
        - kind of brilliant...but also it feels a bit slow. Perhaps that's just the way it is
- for a variable look-up:
    - look it up in the *map*, which holds *local variables only* (or the addr. to them)
    - if it isn't there, we get it from the *globals*
        - this is a *dynamic lookup*
    - if there is a match, we call `getAt` to make sure we go up the proper number of environments
    - the interpreter is *confident* in this lookup
        - the resolver has already visited here; the interpreter would never have known to even check here if the resolver hasn't told it
    > Nystrom notes that for debugging one of these yourself, have the interpreter explicitly assert this "contract" it expects from the resolver.

### Assigning to a resolved variable

- very similar (look up distance) and then use `assign` instead of `get`

## Resolution Errors

- Lox allows declaring multiple variables with the same name *globally* but this is likely an error *locally*
    > this feels...arbitrary. Pick a lane, Nystrom.
- this can be detected and fixed statically
- can check for returns outside of functions, continues and breaks here, etc.
    - I already did breaks and continues though...I think?
    - did these in the parser just checked
