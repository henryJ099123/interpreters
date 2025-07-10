# Ch. 14: Chunks of Bytecode

- jlox's implementation is incredibly slow as it directly walks an AST tree
- implicit reliance on JVM features (exceptions, `instanceof`, stack, etc.)
    - *especially* the garbage collection
- the AST model is *fundamentally* the wrong design; the better one is bytecode

## Bytecode

- alternatives to bytecode:

### what about the AST?

- we *have* already written it
    -very simple to implement
- runtime representation directly maps to the syntax
- portable (because Java, but also in C0
- however, not memory efficient at all
    - every piece of syntax becomes a sea of pointers throughout memory
    - bad for the cache too :(
- this means we want to organize memory the way it's read

### What about to native code, rather than bytecode?

- This is *really* fast and best for the chip
    - this is machine code (or assembly)
    - yikes...I do not want to do all this in machine code
- also no portability
    - there are compilers like LLVM to help though

### So what is bytecode?

- retains portability of a tree-walker
- sacrifices some simplicity to get a performance boost
- structurally, bytecode looks like machine code
    - but it's a *much* higher level instruction set than anything on a real chip
    - ideally, the easiest possible architecture to convert to native (idealized fantasy assembly)
- because this fantasy architecture doesn't exist, we write an *emulator*,
which is a "simulated chip" in software, to interpret the bytecode
    - essentially, a *virtual machine*
    - this emulation makes it slower than going full assembly
- so, we go from **compiler** $\rightarrow$ **bytecode** $\rightarrow$ **VM**
instead of **parser** $\rightarrow$ **syntax tree** $\rightarrow$ **interpreter**

## Finally, getting started

- just basic C files

## Chunks of instructions

- `chunk.c` is a module to define the bytecode representation
- each instruction in the bytecode format has a one-byte **operation-code**

### Dynamic array of instructions

- why Nystrom likes dynamic arrays:
    - Cache-friendly and dense storage
    - Constant-time indexed element lookup
    - Constant-time appending to end of array
- Wow, this really is just data structures all over again
    - add elements into the array if space
    - if no space, must allocate a new array and copy over the elements and delete the old one
    - **amortized analysis**: as long as the array is grown as a multiple of its current size, each append is still $O(1)$
- array will scale by double if not there
    - minimum threshold of 8 bytes, arbitrarily
- `reallocate()` is a function that will take care of all dynamic memory amnagement
- two size arguments get passed to `reallocate`:

|**oldSize** | **newSize** | **Operation**|
-------------|-------------|--------------|
| 0 | Non-zero | Allocate a new block. |
| Non-zero | 0 | Free allocation. |
| Non-zero | smaller than `oldSize` | Shrink existing allocation. |
| Non-zero | Larger than `oldSize` | Grow existing aloocation. |

> What about 0 and 0?

- this function is currently just a wrapper on `realloc`
    - `realloc` is just `malloc` if `oldSize` is just 0
    - be careful for if reallocation is possible!
- note: we only gave in a pointer to the first byte of memory
    - the memory allocator *under the hood* manages more information, such as the size of the memory heap block
    - it can access this information with the pointer to the block to cleanly free it
    - `realloc` updates this size
    - many implementations of `malloc` store the allocated size in memory right *before* the returned address

## Disassembling chunks

- hard to test the code...because we are just moving bytes in memory
- creating a **disassembler**
    - an **assembler** takes human-readable names for CPU into binary machine code (assembly into machine code)
    - a **disassembler** takes machine code and gives out an instruction
    - not useful as much for Lox *users* but good for us developers
        - mirrors the `AstPrinter` class from jlox
- we use `disassembleChunk` to disassemble all the instructions in the chunk
    - the other function `disassembleInstruction` disassembles each instruction in the chunk

## Constants

- can store *code* in chunks, but need to store *data* as well
    - very Von Neumann
    - no syntax tree, so we need to represent the values somehow

### Representing values

- use `typedef` on `double` in case we want to replace our representation of numbers in Lox with something else in C
- for small values, instruction sets tend to store the value in the code stream immediately after the opcode, called **immediate instructions**
    - no good for strings, so those get stored in the data section of memory for regular compilation
    - the JVM has a **constant pool** with each compiled class
- for Lox, we'll do that: have a "constant pool" for all the literals
    - i.e. every chunk will carry a alist of all the literals it has
    - immediates also run into problems with alignment!

### Value arrays

- the constant pool must be dynamic

### Constant instructions

- compiled data/constants needs to be *executed* as well as *stored*
    - need to know when to produce them, so new instruction type
- a single opcode isn't enough to know which constant to load...
    - so, the instruction is allowed to have **operands**, which is binary data *immediately after* the opcode to parameterize the instruction
- we need to specify the operands' format, called the **instruction format**
    - wow, this is literally just comp arch
- for the debugging, we print the opcode, and then the index, and then the actual value

## Line information

- Chunks contain almost all of the info the runtime needs from the source code
    - we need to store the line number too, though, in case of an error
    - in jlox, these were stored in tokens
- given a bytecode instruction, we need to know the line of the user's source program
    - Nystrom's simple but inefficient approach: store a separate array of integers that parallel the bytecode
    - while very simple, it at least keeps the line information in a *separate array* from the bytecode
        - line numbers only needed at runtime, so it shouldn't take up unneeded cache space
- so, with these arrays, we've boiled down the family of AST trees dramatically

## Design note: test your language

- it's extremely important to *test* your language
    - have a comprehensive test-suite
- why?
    - users expect a working programming language
        - last thought is the compiler is bad
    - language implementation is *very interconnected*
    - input to a language is *combinatorial*, meaning an infinite number of programs
    - language implementations are very complex
- what kind of tests? often: "language tests", programs written in the language along with expected outputs/errors
- why these are nice:
    - tests aren't coupled to any internal architectural decisions
    - can use the same tests for multiple language implementations
    - tests can be terse and easy to read/maintain
- why these aren't nice:
    - end-to-end tests help determine *if* there's a bug but not where in the code
    - crafting a good test is hard, especially for an obscure corner of the implementation
    - overhead can be high to actually execute the tests

## Notes from the exercises

- did not do binary search when finding a line number in case instructions are added out of order
- drawbacks of adding a `OP_CONSTANT_LONG` for when the number of constants exceeds 256:
    - makes interpreter more complex
    - uses up an opcode, which might be extremely valuable to the language if it needs them
    - it *might* slow down the interpreter based on cache locality
        - because new function means new code to load in
- none of these are fatal...so not a bad idea
