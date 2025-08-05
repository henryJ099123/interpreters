# Ch. 23: Jumping Back and Forth

- finally, flow control!
- in jlox, a Lox `if` was just a Java `if`, but the CPU does branching
    - this is easy when you already are comfortable with assembly
- the VM's `ip` (or `pc`) field stores the address of the current bytecode instruction
    - so we have full control over this instruction flow
- for an `if` without `else`:
    - if truthy, we continue along
    - if falsey, we skip the code
    - thus, we need to look at the value on top of the stack as our condition
- need to ensure the compiler generates safe code because of the unstructured nature of bytecode

## If statements

- the `OP_JUMP_IF_FALSE` gives how many bytes of code to skip
    - how far to jump?
- we assess how far to jump via a trick called **backpatching**
    - emit the jump instruction first with a *placeholder* operand and track its location
    - compile the 'then' part
    - then go back and replace the placeholder operand with the actual jump distance
- we don't actually use C's `if` statement to do control flow
    - we only use it to determine falsiness and whether to offset the instruction pointer
- note the jump instruction *doesn't pop the condition* (to be fixed later)

### Else clauses

- need to account for else statements, because we can't let the execution code flow through
- so we need another jump from the end of the `then` branch

## Logical operators

- logical operators `and` and `or` short circuit
    - thus, they act like control flow expressions
    - I can adapt this for the ternary operator
    - done!
- should fit a different branch instruction for `or` for simplicity but whatever

## While statements

- *looping* statements jump backwards rather than forwards
- `while` is way easier than `for` so we focus there
- for jumping forward, we emit the instruction in two stages
    - no problem for loops because we jump backwards, not forwards

## For statements

- the `for` loop has three bits, all of which are optional:
    - initializer, run once at the beginning
    - the condition, an expression which makes the loop stop running
    - the increment, which runs at the end of each loop iteration
- in jlox we desugared the for loop into a while loop
- clox will be similar-ish
- the increment is the hardest and strangest part
    - it's a tad convoluted: it appears *before* the body but executes *after*
    - so, we jump over the increment, run the body, and then jump *back* to the increment to run it
- it's convoluted but no need to touch the runtime
    - all gets compiled down to primitive control flow operations the VM supports
- clox is now Turing complete!

## Design note: considering `goto` harmful

- basically, our control flow is just `goto`...but why is this so bad?
    - `goto` can make code unmaintainable, but that was a long time ago
- Dijkstra's paper "slayed" the goto statement
    - programmers write static text but care about its running/dynamic behavior
    - we reason better statically than dynamically
    - so, make dynamic execution reflect the semantic structure as much as possible
- if we ran a program and stopped it, what would we need to know to tell how far along it was?
    - for simple statements, a line number
    - for functions, you would need a call stack
    - for loops, you would need an iteration count (a stack, for nested loops)
- goto breaks this but Dijkstra doesn't explain it well...
- it was proved that *any* control flow using goto can be expressed using sequencing, loops, and branches
    - the clox intrepeter loop implements that unstructured control flow without any gotos
- Nystrom wonders if there are some uses to goto
    - try to break out of a series of nested loops?
    - isn't `break` and `continue` sort of like `goto`?
    - switching inside a loop?
- his main concern is a language design and engineering decision based on fear

## Exercises

1. add `switch` statements to clox with the following grammar:
```
switchStmt -> "switch" "(" expression ")" "{" switchCase* defaultCase? "}" ;
switchCase -> "case" expression ":" statement* ;
defaultCase -> "default" ":" statement* ;
```
    - to execute a `switch`, evaluate the parenthesized switch expression
    - then walk the cases: for each case, evaluate it; if equal, execute the statement under the case
        - then, exit the `switch`
    - otherwise, try the next case
    - omit fallthrough and `break` statements; just jump to the end of the switch
    - this is just a glorified `if...else if...else if`
    - ok twas a bit harder than I expected but it's DONE!

2. add `continue`, which jumps directly to the top of the nearest enclosing loop.
i.e. it skips the rest of the loop body. In a `for` loop, it jumps to the increment clause.
It is a compile-time error for a `continue` statement *not* in a loop.
Be sure to think about *scope*. What should happen to local variables declared inside the loop
body or in blocks nested inside the loop when a continue is executed?
    - I would think you would just jump to the top, or to the loop jump statement
    - but...what's all this about local variables?
    - they should just vanish and start over...
    - which means if there's a scope change I need to downgrade a scope 
    - done! And added `break` statements as well

3. Control flow constructs have been pretty set-in-stone.
Code has focused on being more declarative and high level, not on imperative control flow.
Try to invent a useful, novel control flow feature for Lox.
In practice, this isn't usually *that* helpful, as users in essence must be forced
to learn an unfamiliar notation, but it's a good chance to practice design skills.
    - This could just be my `aftereach` implementation...which I *could* do now
    - I might save this for later
