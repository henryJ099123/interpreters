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
