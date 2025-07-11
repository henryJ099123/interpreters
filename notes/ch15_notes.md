# Ch. 15: A Virtual Machine

- the VM is made to execute the bytecode instructions

## An instruction execution machine

- the VM is one part of the interpreter's internal architecture
    - give it a Chunk and it runs it
- the VM is its own struct declared as `vm` in a global context
    - treats it as a singleton to simplify function declarations
    - note: for realsies, not a smart engineering choice
        - if this is intended for embedding in other applications, the host can have more flexibility with a VM pointer
        - lets host app control when/where VM memory is allocated

### Executing instructions

- the VM stores the current spot in the code in an `ip` variable, which is just a program counter
    - this should be stored locally because it gets modified remarkably often
    - however, we do not, because other functions will need it too
- faster to dereference a pointer than look up an element in array by index
- `ip` stands for **instruction pointer** (it's just the program counter)
    - points to the instruction *about to be executed* (the *next* instruction)
- to process an instruction:
    - read from the PC and decode the opcode (known as **decoding** or **dispatching**)
    - this is done for every single instruction, so very critical spot for performance;
    some techinques to handle this:
        - "direct threaded code"
        - "jump table"
        - "computed goto"
- the fastest solutions to decoding require nonstandard extensions to C or assembly code
    - just use a big `switch` for simplicity
- use macros for `READ_BYTE` and `READ_CONSTANT` and define them only inside of `run` to make the scoping explicit

> ***Weird error***: the VM doesn't quit if there's no return at the end...this is literally how assembly works lol

### Execution tracing

- the VM only works because of temp code to log the value
    - after doing this, the VM is a black box
- therefore, good diagnostic logging would be a good idea
- notice how much simpler, and thus faster, this is than walking the abstract syntax tree
    - and that stupid Visitor pattern whatnot

## A Value Stack Manipulator

- Lox has expressions like produce/modify/consume values
```
print 3 - 2;
```
- This requires instruction for `print`, 3, 2, and subtraction, but how can a subtraction instruction know it's doing 3 minus 2?
```
fun(echo(n)) {
    print n;
    return n;
} 

print echo(echo(1) + echo(2)) + echo(echo(4) + echo(5));
```
- left operands executed before right ones, so this must print:
    - 1 2 3 4 5 9 12
> Note that some languages let the implementation decide evaluation order for efficiency, like C
- In jlox, the syntax tree was recursively traversed in a postorder fashion (LRC)
    - this accomplished the intended behavior
    - how do we accomplish the same thing in C?
- step through that above program one at a time:
    - constant 1
    - `echo(1)`
    - constant 2
    - `echo(2)`
    - 1 + 2
    - `echo(3)`
    - constant 4
    - `echo(4)`
    - constant 5
    - `echo(5)`
    - 4 + 5
    - `echo(9)`
    - 3 + 9
    - `print 12`
- notice that the 1 and 2 are unneeded after 1 + 2, and the 4 and 5 are unneeded after 4 + 5
    - notice that whenever a number "appears" earlier than another, it lives at least as long as that other one, because it is later consumed
    - basically this is a stack
    - the 1 is added, then 2, then those are consumed for 3; then 4 and 5 added, consumed to 9; 9 and 3 consumed to 12

### The VM's Stack

- stack-based execution is very, very simple (apparently)
- not the stack pointer is one *above* the stack, i.e. where the next pushed element is
    - apparently, the C standard allows a ptr to point just past the end of a full stack
- fixed size stack: possible to stack overflow, but for simplicity not dynamically growing the stack
- no checking for beginning or end of stack?

### Stack tracing

- when tracing execution, we show current contents of the stack before interpreting

## An arithmetic calculator

- now it's time to do some arithmetic, starting with unary negation
    - no parser, but we can add the bytecode instruction)

### Binary operators

- nothing very special 
- a bit risque for a use of the C preprocessor
    - can pass *operators* to a macro?? It *is* just text...
    - I suppose you *have* to wrap this in a `do...while` to make a single statement
- ope Nystrom explained it right after the use...lol
    - just a way to package multiple statements while allowing a concluding semicolon
- note that the *order* of popping gets `b` before `a` (which it has to...because it's a stack)
- for debugging, the *operators* take operands but the *bytecode instructions* don't, so the debugging doesn't read more bytes
    - bytecode instructions read from the stack

## Design note: register-based bytecode

- we *could* have done *register-based* bytecode instructions instead of stack-based
    - register-based VMs still have a stack, especially for temporary values,but...
    - instructions can read inputs from anywhere in the stack and store their outputs into specific stack slots
```
var a = 1;
var b = 2;
var c = a + b;
```
- this will (for us) be:
```
load &lt;a&gt;
load &lt;b&gt;
add
store &lt;c&gt;
```
- four separate instructions and seven bytes of code (four opcodes, three operands)
- register based encoding would look like this:
```
add &lt;a&gt; &lt;b&gt; &lt;c&gt;
```
- literally just an assembly thing
- reads directly from `a` and `b` and store it in `c`
    - single instruction, four bytes
    - net win despite the increase in complexity
- Lua's transition to this style from the stack-based style imrpoved their speed
- we are doing stack-based because it's simpler and more widespread in literature and community

## Exercises

1. in broad strokes:
    - ld 1, ld 2, multiply, ld 3, add
    - ... skip to the last
    - ld 1, ld 2, ld 3, multiply, add, ld 4, ld 5, negate, divide, subtract
2. answer:
    - eliminating negate: ld 4, ld 0, ld 3, subtract, ld 0, ld 2, subtract, multiply, add
    - eliminating subtract: ld 4, ld 3, negate, ld 2, negate, multiply, add
    - I'd say it doesn't *not* make sense, but I'd probably get rid of OP\_SUBTRACT if I had to pick
    - maybe add instructions for incrementing or decrementing?
3. The VM is kind of small...so no check that a value overflows it
    - could crash the VM...so dynamically grow the stack
    - I know how to do this (add ptr to max capacity...check if equal...allocate more space), but I don't really want to? Feels like an annoying idea
4. Interpreting `OP_NEGATE` pops the operand, negates the value, and pushes the result, but this is slow...optimize it
