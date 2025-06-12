# Ch. 8: Statements and State

- we need to add *state* to the interpreter (i.e. variables and such)
- *statements* generate **side effects**, something that modifies internal state or does something other than generate an output
    - I/O
    - variables

## Statements

- extend Lox's grammar with statements
1. **expression statement**: place expression in place of statement
    - evaluate expressions with side effects
2. **print statement**: evaluates experssion and displays result
    - this is now abnormal. Most print statements are library functions and not built right into a language
    - product of the fact this book is built slowly and that the language is primitive
- as Lox is imperative and dynamically types, the higher levels of the grammar are just statements
    - i.e. we add
    ```
    program -> statement* EOF;
    statement -> exprStmt | printStmt;
    exprStmt -> expression ";" ;
    printStmt -> "print" expression ";" ;
    ```
    to our grammar.
    - mandatory `EOF` token indicates the parser consumes the entire input
- we will store the grammar into syntax trees now

### Statement syntax trees

- expressions and statements do not overlap
    - operands to `+` are expressions; body of a loop is a statement
- thus, we may *split* the two into separate syntax class hierarchies

### Parsing statements

- `parse()` was a bit hacky before (returns a single expression)
    - with statements this is now a bit more robust
    - i.e. the parser now parses a bunch of statements which may contain expressions

### Executing statements

- now we just have to interpret the statements with the `Interpreter` class
    - now it will also implement the `Stmt.Visitor` interface as well
    - statements do not produce values (only side effects)
- fairly simple reorganizing to allow statements to occur

## Global variables

- **variable declaration**: statement that adds a new variable
```
var beverage = "espresso";
```
- **variable experssion**: statement that accesses the variable binding
```
print beverage;
```

### Variable syntax

- plan: split the statement grammar in two to handle variable stuff from other stuff
- grammar restricts *where* some kind of statements are allowed
- ex. clauses in simple control flow statements. Consider the following:
```
if (work_time) print ":(";
```
versus
```
if (work_time) var myfeelings = "very sad";
```
The first is sensible, but the second is weird. Thus we don't allow it, because the scope of this variable is bizarre.
    - the solution to this is *block statements*, coming later
        - blocks : statements, parentheses : expressions
- must modify the grammar to handle this by having declarations be lower precedence, and thus can go into other forms of statements

### Parsing variables

- now can include parse error recovery (the synchronization)
    - `declaration()` method will be called a bunch of times to parse a series of statements, so it is ideal there to synchronize when the parser panics
        - i.e. skip to the next line
    - if `declaration()` cannot find a valid declaration, we go to `statement()`; if that cannot find a valid statement, we go to `expression()`; if that cannot parse the expression at the current token, we get a syntax error
        - so if something cannot be parsed, we get a syntax error!
- just have the grammar be more explicitly written out here in code
    - have the parser also return identifiers (variable names) to the interpreter for it to handle

## Environments

- The bindings for associating variables needs to live in memory somewhere, called the **environment**
    - seems like a hashmap for variable names and values, which is how Nystrom implements it in Java
- using Strings as the keys, not tokens, becuase tokens are unique to where they are in the source text, but the same variable being accessed should go to the same data (ignoring scope)
- need to support *defining* a variable
    - This implementation allows *redefining* variables with the `var` keyword
        - nicer with the `REPL` to not have to think about that
- need to support *accessing* a variable
    - what if the variable doesn't exist?
        - syntax error: sooner error detection
    - however, *using* var doesn't mean *referring* to it
        - what if the variable is in a function?
        - what if two functions call each other? neither can be first
        > recall C and function headers. Some languages like Java assert the top level of a program is not imperative but declares all names before looking at bodies
    - thus, we use runtime error; i.e. OK to refer to a variable before it is defined as long as that reference is not evaluated beforehand
- need to support *writing* to a variable

### Interpreting global variables

- we give the interpreter an instance of this `Environment` class
- what to do about uninitializing a declared variable?
    - don't make an error, many languages are OK with this
    - usually, accessing it would be a runtime error, but dynamically typed languages instead assign it `null`, so we will assign it `nil` if uninitialized
- when you inevitably get stuck on this later:
    - the purpose of `Expr.Variable` is for when `primary()` (the lowest part of the grammar, highest precedence) has a *variable name*
        - when this is found in some expression by the parser, the parse tree contains an instance of the class here
        - when it is found by the interpreter, it just substitutes in its corresponding value

## Assignment

- some languages do not allow variable reassignment or **mutation**
    - e.g. Haskell, discouraged by Rust
- Lox is not one of these. lol
- just need assignment notation with the `=`

### Assignment snytax

- assignment is an *expression*, not a *statement*
    - lowest precedence expression form (well, except for comma expressions, and conditional expressions (ternary operator))
    - therefore must update the grammar
- problem: a single token lookahead recursive descent parser (mouthful) cannot tell its parsing an assignment until *after* it parses the left-hand-side and sees the `=` sign
    - while `a + b` has a left-hand-side that evaluates to a value, in `a = b`, the left-hand-side isn't evaluated, but assigned to
- **r-value**: an expression that produces a value
- **l-value**: an expression that evaluates to a storage location that you assign something to
    - the `Expr.Assign` node has a `Token` for its left-hand side instead of an `Expr`
    - we also only have a single token of lookahead. what to do?
- just evaluate the lefthand side. if its a variable type, then just do variable assignment
    - report the error if the LHS is not a valid assignment target
    - slightly different: do not loop to build up a sequence of the same operator like below, rather we just recursively call `assignment()` to parse the RHS
    - this works because we know what the assignment target is like by evaluating the LHS
    - every valid assignment target happens to be valid syntax as a normal assignment; parse it as if it were an expression
        - this means if it is fundamentally NOT a regular old variable, like `a + b = c` or `(a) = c`, we get an error, because it is NOT `Expr.Variable`

### Assignment semantics

- now we have to update the interpreter
    - if assigning to undeclared variable, that's a Runtime error
    > this isn't Python.
- I modified things to allow the ternary operator and the comma operator for expressions.
    - I did not follow C's precedence hierarchy because I think it's dumb that I can't make a statement that looks like
    ```
    true ? a = b : a = c;
    ```
    and have it work properly. So I made it have the same precedence as assignment.

## Scope

- **scope**: region where a name maps to a certain entity
    - multiple scopes: same name can refer to different things in different contexts
- **lexical scope** (or static scope): style of scoping where the text of the program itself shows where the scope begins and ends
    - this is the case in most languages for variables
    - e.g. `{}`
- **dynamic scope**: unknown what a name refers to until the code is executed
    - Lox has dynamically scoped methods and fields on objects
    - consider the following block:
```
class Saxophone {
    play() {
        ...
    }
}

class GolfClub {
    play() {
        ...
    }
}

fun playIt(thing) {
    thing.play();
}
```
- it's unknown whether `thing` will play the `Saxophone`'s thing or the `GolfClub`'s thing
- scope is the theoretical counterpart to environment
    - Lox's scope is controlled by `{}` (called **block scope**)

### Nesting and Shadowing

- at first glance we might
    - keep track of any variables declared
    - delete all variables when the last statement is executed
- this would work except local scope when variables have the same name
    - local variables can **shadow** outer ones by sharing the same name
- interpreter may need to look in any outer environments for a more "global" variable
    - so environments are *chained* together so inner ones have reference to the other one
    - known as a **parent-pointer tree** or a **cactus stack**
- blocks take the place of statements, so in the grammar a block replaces a statement
    - block is a collection of declarations surrounded by curly braces
- Nystrom manually changes and restores a mutable `environment` variable rather than just passing it as an argument
    - did this only for simplicity

## Design note

- Lox has distinct syntax for declaring a variable and for assigning to an existing one
    - Python doesn't have this; it's called **implicit variable declaration**
    - must deal with shadowing (some explicitly disallow, others create new variables)
- explicit declaration lets the compiler know what type each variable is and how much storage should be allocated
    - not really necessary in dynamically typed or garbage collected languages
- problems with implicit declaration:
    - intent to write to existing variable is not noticed if misspelled
    - adding new variables in a local scope affects the meaning of existing code with certain languages
    - assigning to a variable outside of scope can be difficult
- to fix these, things become...less simple
    - `global` and `nonlocal` in Python
