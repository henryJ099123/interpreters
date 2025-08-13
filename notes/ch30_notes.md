# Ch. 30: Optimization

- The last chapter!
- pure performance gains in this chapter

## Measuring Performance

- **optimization** is taking a working application and improving its performance
    - an optimized program does the same thing with less resources
- there used to be a time when a skilled program could just think hard to
come up with performance gains
    - this is gone with so many abstractions between hardware and software
    - even C is not really low level
- different Lox programs will stress different parts of the VM,
so different architectures have their own strengths and weaknesses

### Benchmarks

- we validate correctness by writing tests
    - Lox programs that use a feature and validate the VM's behavior
- tests pin down semantics to ensure *not* breaking existing features
- how do we validate than an optimization actually *does* improve performance?
    - how do we ensure that the other unrelated changes don't *regress* performance?
- accomplish benchmarking goals with **benchmarks**
- don't measure the capability of a program but the time it takes to do so
- once there's an entire *suite* of benchmarks, you can measure
not just *that* an optimization changes performance, but on which *kinds* of code
- the suite of benchmarks you write are a key part of that decision

### Profiling

- To make the benchmarks go faster, supposing you've used the right algs and data structures,
you have to use a **profiler**
    - a profiler is a tool that runs your program and tracks hardware resource use
    - simple ones show time wasted on each function in the program
    - sophisticated ones show cache misses, branch mispredictions, and more
- it's worth getting familiar with a decent profile

### Faster hash table probing

- accesses and method calls are usually a large fraction of user code in Lox
    - benchmarks must be careful to *use* the result of the code
    - compilers do aggressive dead code elimination
    - is your program fast, or is your compiler just super optimized?
- make a few guesses: for a program with many accesses/method calls, where does
the VM spend most of its time?
    - naturally, in `run()` for its **inclusive time**
    - but really, in `tableGet()`, which takes 72% of the execution time
- in `tableGet`, almost *all* of the execution time is spent on *one line*:
the one with the modulus, the %
    - modulo is really slow
    - it's hard to reimplement a fundamental arithmetic operator than the CPU can do
    - however, we know more about our problem than the CPU does
- we know that the table's size is *always* a power of two
    - a faster way to calculate a remainder of a power of two is by bit masking
- this actually was faster. It improved the speed by like 33%. 
- the point is we didn't *know* the % was the problem until the profiler said so

## NaN Boxing

- this optimization is more subtle than the last and the profiler won't help
- this optimization is called **NaN boxing** or **NaN tagging**
- On a 64-bit machine, Value takes up 16 bytes
    - struct has two fields: a type tag and a union for the payload
    - the largest field in the union is the Obj pointer and a double, which are both 8 bytes
    - for alignment, four bytes are added in the middle by the compiler
- saving space here can mean fewer cache misses, which dramatically increases speed
- If Values need to be aligned to largest payload size, and a Lox number or Obj needs
a full 8 bytes, can this even *get* any smaller?
    - annoying for dynamic language makers, because static languages do not have this problem
    - the type of a piece of data is known at compile time, so extra memory isn't needed
    to track it
- NaN boxing is a trick to combat this, espsecially for languages which store all numbers as 
double-precision floating point

### What is (and isn't) a number?

- how are doubles stored?
- almost all machines use the IEEE 754, the IEEE Standard for Floating-Point arithmetic
- an 8 byte double-precision floating-point number looks like this:
    - bits [0:51] are the **fraction** or **mantissa** (or **significand**) bits
        - they represent the significant digits of the number, as a binary int
    - bits [52:62] are the **exponent** bits, which show how far the mantissa
    is shifted away from the decimal point
    - the last bit is the **sign bit**

    > There's even a positive and negative 0!

- the important part: there's a special case exponent
    - when *all* of the exponent bits are set, the value is known as "Not a Number",
    hence **NaN**
- *Any* double whose exponent bits are all set is NaN regardless of the mantissa bits,
so there's lots of *different* NaN bit patterns
    - values where the highest mantissa bit is 0 are called **signalling NaNs**,
    and the rest are called **quiet NaNs**
    - signaling NaNs are supposed to be the result of bad computations, like division by 0
        - you are NOT supposed to use these values
    - quiet NaNs are supposed to be safer to use
- Every double with all of its exponent bits set and its highest mantissa bit set
are quiet NaNs. And there are 2^51 of these, as there are 51 bits left unaccounted
    - actually 52, but avoid one of those to not deal with some Intel whatnot
- this means thata 64 bit double has enough room to store all of the various
different numeric floating-point values and *also* room to set aside some bit patterns
to represent Lox's `nil`, `true`, and `false` values
- but what about the Obj pointers?
- technically pointers 64 bits, but no architecture actually uses the entire address space
    - most chips only ever use the low 48 bits
    - the remaining 16 bits are either unspecified or always 0
- if there's 51 bits, it's possible to stuff a 48-bit pointer in as well with 3 bits to spare
    - those 3 bits are enough to store tiny type tags to distinguish between `nil`,
    Booleans, and Obj pointers
- within a single 64-bit double, you can store all of the different floating-point numeric
values, a pointer, or some other special sentinel values
    - there is no need to convert a numeric double value into a "boxed" form
        - the numbers stay the same
- we still need to check type beforehand, but since Lox is dynamically typed,
there's no work to go from value to number...there is for the other types though

### Conditional support

- NaN boxing relies on some low-level details of how a chip represents floating-point numbers
and pointers
    - it *probably works* for most CPUs but not guaranteed to be so for all
- so we maintain support for both
    - just need to replace macros for the Value conversions with lots of bitwise operations

### Numbers

- numbers have the most direct representation under NaN boxing
    - the representation is exactly the same
    - but we need to convince the C compiler this is the case
- **type punning**: interpreting the same bits as one data type versus another
    - spec authors do not like type punning because it makes optimization harder
    - compilers like to assume **strict aliasing** for pointers to ensure that
    pointers of different types do not point to the same thing
    - but we intend to break that
- one way to convert that *is* supported by both C and C++ specs
    - via `memcpy`? This looks horrendously slow
    - most compilers recognize this pattern and optimize away the `memcpy`

### nil, true, false (and undef)

- these are all singleton values
- claim the lowest two bits of the unused mantissa space as a "type tag"
to determine which of the singletons we're using
    - these are bits 0 and 1 of the entire double

### Objects

- there are billions of pointers that need to be boxed in a NaN,
so we need a tag to indicate that these NaNs *are* Obj pointers
- the tag bits for the singleton are in the region for the pointer itself,
so a different bit there can't be used to determine an object reference
- we can use the sign bit as the type tag for objects
    - we actually *could* use the lowest bits as a type tag because
    Obj pointers are 8-byte aligned, and since most addresses are byte addressable,
    the lowest 3 bits of the pointer will be 0
    - this is known as **pointer tagging**
- note that `==` for doubles as numbers is mostly correct
    - most floating point numbers with different bit representations are distinct
    - however, the IEEE spec mandates that NaN values are *not equal to themselves*
    - this would be a problem should we produce "real" arithmetic NaN in Lox

### Evaluating performance

- has this actually *helped*?
- the effects of changing this value representation is more diffused
    - the macros are expanded in place wherever used,
    so performance is spread across the codebase that's hard for profilers
- when doing profiling work, you often want to profile an optimized
"release" build of the program since that reflects true performance
    - compiler optimizations can dramatically affect which parts of the code
    are performance hotspots
    - hand-optimizing a debug build risks "solving" a provlem that the
    optimized compiler already does
    - make sure you don't accidentally benchmark and optimize the debug build!
- hard to *reason* about the effects of the change
    - smaller values $\Rightarrow$ reduced cache misses
    - actual real-world performance effect of that change is platform dependent 
- basically everything gets a little faster (it does...I've tested it)
- but, it is possible that the bitwise operations nullify the gains from the better
memory use
    - performance work like this is unnerving because you can't *prove* the VM is better
- instead, what you need is a large *suite* of benchmarks
    - on Nystrom's machine, this seems to lead to a 10% improvement

## Where to next

- what's the next step for me?
- compilers/interpreters is a small slice of both academia and industry
- general skills taken away from this book and solving problems
- some of Nystrom's suggestions for moving forward:
    - this book pushed towards runtime optimization, but
    *compile-time optimization* is a very rich field and generally more important.
    Classic compilers books will cover this.
    - Dynamic typing places some restrictions, but maybe add a static
    typing check.
    - There's a whole world of programming language academia.
    These have been studied foramlly for ages.
    Look for parser theory, type systems, semantics, and formal logic.
    - Turn Lox into your own plaything.
    - Distinct world of programming language *popularity*:
    writing documentation, example programs,
    tools, useful libraries.

## Exercises

- Fire up the profiler and look for other VM hotspots.
- Many strings are really small in real world programs.
Most VMs don't intern strings, either. A classic trick for these
is to have a separate value rpresentation for small strings that stores
the characters inline in the value.
    - implement that optimization
- Reflect on this book?

