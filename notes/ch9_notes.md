# Ch. 9: Control Flow

## Turing Machines (lol)

- Turing's **Turing machine** and Church's **Lambda calculus** deal with the idea of if things are computable
    - need arithmetic, control flow, and memory

## Conditional Execution

- **conditional/branching control flow** is a way to not execute code
- **looping control flow** executes some code multiple times
- Lox *does* have a conditional operator, actually, because I implemented it, so now I have to deal with that
- Note: for `if` statements, you need a delimiter between the expression and the `then` block, but there's no need for a delimiter between the `if` and the condition
    - i.e. the `(` in `if(expression)` is unnecessary
- an optional `else` introduces ambiguity
```
if(first) if(second) whenTrue(); else whenFalse();
```
- is the `else` clause here for the first or the second `if` statement?
    - this is known as the **dangling else** problem
    - "fixed" by kind of ignoring it, and having the `else` go to the most recent `if` (the one that directly precedes it)

## Logical Operators

- we still need to implement `and` and `or`, and these are special because they **short-circuit**
    - short-circuiting is not a *syntax* concern but a *semantic* one
- Nystrom decides to define a new class. I think this is bloated, but whatever
    - he also decides that the logical 'and' and 'or' operators `and` and `or` do not return Boolean values but something of a proper *truthiness*
    - I didn't do this when I implemented `xor`. I don't logically see the point of returning something that isn't just `true` or `false` because there's no real scenario where you'd want to use logical `XOR` and *not* get true or false as an output.
    - I *do* see a point with *bitwise* xor, but I didn't implement that

## While loop

- we have `while` loops and `for` loops
- recall the for loop:
    - initializer (executed exactly once)
        - usually an expression or a variable declaration
    - condition
    - increment
- any of these statements can be omitted
- note that this `for` loop doesn't actually *add* anything to the language
    - it is simply syntactic sugar
    - **desugaring**: translate syntax sugar to more primitive form to use already existing code
    - i.e. convert `for` into `while`
        - I did this for the pre-incrementers `++` and `--`
    - I think I am going to add the ability for a "finally" aspect to the `while` loop (hidden) so that the execution of the increment for `for` still occurs regardless of the body having a `continue`
        - so I did this and give option for an `aftereach` to execute each loop regardless of `continue` but *doesn't* execute on `break`

## Design note on syntactic sugar

- how much syntactic sugar to put into a language?
    - some languages push for almost no syntax at all, like Lisp and Smalltalk
        - pushes for the *language* to not need syntactic sugar and let library code be as expressive as the language
    - some languages prioritize simplicity and clarity, such as C, Lua, and Go
    - some languages have a mix of both, like Java and Python
    - some languages heap on syntactic sugar, like C++ and Perl
- to a degree, later released languages have more syntactic sugar, and newer syntax is nice sometimes as it can be backwards compatible
    - once added, it can't be removed!
- there's a popular push against syntactic sugar, as it can bloat a language, but it seems successful langauges have a good amount of syntactic sugar

