# Ch. 21: Global Variables

- in jlox, we used a chain of environments to store variables and manage state
    - unfortunately it is very, very *slow*
    - a new hash table for *every block* and call
    - variables slow $\rightarrow$ everything slow
- recall global variables in Lox are resolved dynamically
    - can compile code that refers to a global variable early
    - i.e. can refer to later variables inside the body of functions
- local variables, rather, must *always* be declared before use

## Statements

- Variables need variable declarations $\Rightarrow$ statements
- Lox split statements into declarations and everything else
    - disallow declarations immediately inside control flow statements
- we can now implement a series of declarations in a full implementation
- Nystrom swears having this many helper functions helps with readability
- note that the print statement does not push anything!
    - every bytecode instruction has a **stack effect** that describes how the instruction modifies the stack
        - e.g. `OP_ADD` pops two things and pushes one
    - every expression leaves some result on the stack, but an entire statement has a total stack effect of zero
    - i.e. leaves the stack unchanged, tho may use it as a side-effect
- an expression statement just throws away semantically whatever it made, so we just pop it
    - typically these ones will have their own side-effects

### Error synchronization

- clox uses panic mode error recovery, and it exits panic mode at the semicolon, the "error sync" point
    - this is to minimize the number of cascaded errors

## Variable declarations

- these require three operations:
    - declare variable with `var`
    - access value with identifier expression
    - store value in existing variable with assignment
- the compiler desugars
```
var a;
```
into
```
var a = nil;
```
- okay...so now the stupid constant business is coming back to bite me in the behind
    - the exercises in this bookare *not* well designed, unfortunately
- we add the string of the variable name in the constant table and get its index
    - this is added to the values array, and then added to the end of the stack
    - then we eat this by adding it to the hash table for global variables
- we pop *after* adding to the hash table so garbage collecting can work correctly

## Reading variables

- access the variable's value via a name
    - similar process as before

## Assignment

- Nystrom's choices for the bytecode compiler have led assignment to be...a piece of work
- the bytecode VM uses a single-pass compiler without intermediate AST
    - assignment doesn't really fit this. Consider this:
    ```
    menu.brunch(sunday).beverage = "mimosa";
    ```
    - we don't know this is assignment until the `=` and by that point the compiler's done a ton of other bytecode stuff
- for that example, the `.beverage` can't be a get expression, but everything left of it is
    - i.e. `menu.brunch(sunday)` can be executed normally
    - so, there isn't a ton of lookahead to know `beverage` should be a set, not a get, expression
- so, right *before* compiling an expression that may also have an assignment target, we look for `=`
    - then if it's there we compile as an assignment or setter
    - otherwise, we compile as access or getter
- if the variable doesn't exist, we need to add and then delete it (feels a bit inefficient)
- note that setting a variable does NOT pop anything off the stack
    - assignment *returns* something, remember?
- small issue:
```
a * b = c + d;
```
This should be a syntax error, as `=` has lowest precedence
    - i.e. `a * b` can't be an assignment target.
- here's what the parser does:
    - `parsePrecedence()` parses `a` using the `variable()` prefix parser
    - reaches the infix parsing loop
    - reaches `*`, calls `binary()`
    - recursively calls `variable()` to parse `b`
    - inside the call to `variable()`, looks for `=`, and parses the rest of the line as assignment
- i.e. `variable()` does not take into account the precedence of the surrounding expression
    - so, this should only consume the `=` if it is in a lower precedence
- we need to pass this as a flag...but as `variable` is in a table of functoin pointers,
all of the parse functions need to have the same declaration
    - even if they don't need it at all...only setters (and when I add subscripts, `[index]`) do

## Exercises

1. The compiler adds a global variable's name to the constant table
as a string each time it encounters an identifier.
This constant is made *each time*, even if the variable name is already
in the constant table. This wastes memory.
Optimize this. How does your optimization affect the performance of the compiler, compared to runtime?
Is this a good trade-off?
    - One way of optimizing I can see here is to use a hash table as a set for vars
        - Results in constant-time addition of each global variable
        - Is the overhead of a *separate* hash table just for the global really a good idea?
    - Another way of optimization is to simple search the Values array
        - not that slow, as each string can be compared via a direct pointer equality
    - So I was right about both of these. Nystrom implements the first...so I'll give it a shot
