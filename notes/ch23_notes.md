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
