# Ch. 26: Garbage Collection

- Lox is "high level" because programmers need not worry about irrelevant details
- dynamic memory allocation is perfect for automation
    - necessary for a working program
    - annoying
    - tedious
    - error-prone
- Lox is a **managed language**, i.e. the implementation manages memory allocation
    - the programmer never has to deallocate anything
    - provides illusion of infinite memory
    - to support this illusion, we reclaim done-with memory: the **garbage collector**

## Reachability

- how does a vM tell what memory is *not* needed?
    - memory is only needed if it is read *in the future*...but we have no time machine
- so, languages make approximations for memory that *could possibly* be read in the future
    - a **conservative GC** is a special collector that considers every piece of memory
    to be a pointer if the value in the memory could be an address
    - a **precise GC** knows exactly which words in memory are pointers and which store
    other pieces of data
- but couldn't *any* bit of memory be read?

```text
var a = "first value";
a = "updated";
// garbage collector
print a;
```

The string "first value" is in memory here, but the user's program can never access it again
    - the string can safely be freed because the program lost any reference to the string
    - this value is **unreachable**, unlike "updated", which is **reachable**

```text
var global = "string";
{
    var local = "another";
    print global + local;
} 
```

If we look at the VM right before printing the concatenated string,
    - it can access "string" via `global`, "another" via `local`, and the concatenated
    value be the stack
    - these are called **roots**, which are objects the VM can reach directly
- other values can be found through a reference inside another value, such as fields on an object

```text
fun makeClosure() {
    var a = "data";

    fun f() { print a; }
    return f;
} 

{
    var closure = makeClosure();
    // GC here
    closure();
} 
```

If we pause on the line with the garbage collector, it will call the closure, which prints "data"
    - so this string "data" can't be freed
    - but "data" isn't even on the stack, and has been hoisted into the closure upvalue
- thus, we have an *inductive* definition of reachability:
    - all roots are reachable
    - any object referred from a reachable object is itself reachable
- anything that doesn't fit this defintion is free for the garbage collector to collect
- recursive algorithm to free memory:
    - starting with roots, traverse through object references to find the full set of reachable objects
    - free everything *not* in this set

## Mark-sweep garbage collection

- Lisp was the first managed language
- simplest algorithm is called **mark-and-sweep**, or just **mark-sweep**
    - **marking**: start with roots and traverse/*trace* all objects referred to by those roots
        - a classic graph traversal
        - we *mark* each node visited
    - **sweeping**: every reachable object in the heap has been marked
        - any *unmarked* object is reachable and reclaimable
    - this is in contrast to a **reference-counting gc**

### Collecting garbage

- collecting before allocation is a classic method for wiring a GC into a virtual machine
    - this is also the only time you actually need memory
    - more sophisticated collectors run on separate threads or interleave periodically
    usually at call boundaries or backward jumps

### Debug log

- GCs are opaque...how to tell it's actually working?

## Marking the roots

- as objects are scatted around, we begin marking at the roots
    - most of these are locals/temps on the VM's stack

### Less obvious roots

- the VM has a few places that it hides references to values that it *can* directly access
- most of a function's state is on the value stack, but there's the CallFrame stack, too
    - each of these contain a pointer to the closure being called
- allocations can happen during the compilation period as well, so if the GC runs in compilation,
any values the compiler directly accesses needs to be treated as roots as well

## Tracing object references

- there aren't *many* objects that contain references, but there are some
    - e.g. the upvalues list in each ObjClosure, and the raw function it wraps
    - e.g. the constant table in each ObjFunction via the Chunk
- for traversal, we can do breadth or depth first, but as we need the *set* of all objects,
the order does not matter that much

### the tricolor abstraction

- we need to ensure the collector does not get stuck in its graph traversal
- we use a **tricolor abstraction** we assign to each node
    - white: every object is white at beginning of garbage collection
    - gray: during marking, darken to gray; object is reachable but have not traced through its
    references
        - have not seen its *worklist* yet
    - black: marked an object and all of its references
- note that a black node cannot point to a white one: this is a **tricolor invariant**

### A worklist for gray objects

- we've marked the roots in our implementation, but now we need to traverse their references
- unfortunately, we don't have an easy way to find the marked objects so far
    - create a separate list just for these
    - Nystrom uses a stack, but managed by the *system*, not ourselves
        - can't have the GC recursively call itself
- we thus handle allocation failure by exiting
    - more robust would be to allocate a rainy day fund block of memory at VM start
    - we then free that block if we run out of space for the gray stack allocation

### Processing gray objects

- do different things based on each object

## Sweeping unused objects

- now the GC has all of the objects attainable
    - must sweep all of the white nodes and reclaim them


### Weak references and the string pool

- recall that strings are interned
    - the VM has a hash table for every string in the heap
    - for de-duplicating strings
- we deliberately did *not* treat the string table as a source of roots
    - if we did, no string would ever be collected
    - the string table would continue to grow
- however, if we *do* let the GC free strings, 
the VM's string table will be left with dangling pointers
- thus, the string table is special and needs a special reference
    - the table needs to refer to a string,
    but it cannot be considered a root
    - however, the referenced object *can* be freed,
    and thus the dangling pointer must be removed
    - this is called a **weak reference**
- not including the string table in marking directly means they
aren't considered as a root
    - however, we must clear dangling pointers
    - must know the strings that *are* unreachable but fix this pointer business
    before the sweeping

## When to collect

- problem is now deciding *when* the collector should be invoked in normal program execution
    - this is a question poorly answered by most literature secondo Nystrom
    - early garbage collectors worked on small memories and thus operated when memory ran out
- modern machines have such large memories that you never run out, but just get slower and slower
because of page faults

### Latency and throughput

- every managed language pays a performance price compared to user-authored deallocation
    - the GC spends cycles figuring out *which* memory to free
    - i.e. the mark phase
- **throughput**: total fraction of time spent running user code versus garbage collection
    - most fundamental measure: tracks total cost of collection overhead
- **latency**: longest continuous chunk of time where the user's program is paused
during garbage collection
- an obvious implementation is separate threads: a **concurrent garbage collector**
    - this is how very sophisticated garbage collectors work
    - this requires coordination, though: much more overhead and complexity...
    challenging to implement
- the garbage collector in this book is a **stop-the-world garbage collecotr**, which means
the user's program is *paused* until the entire garbage collection process has completed
    - waiting too long to run the collector runs up the number of dead objects
    which increases latency
    - this is in contrast to an **incremental garbage collector** that can do a little collection
    at a time
- but running the collector really frequently spends time visiting live objects
- so, we want something in the middle

### Self-adjusting heap

- how to balance between latency and throughput?
    - could let the user pick by exposing garbage collection parameters,
    but we should be able to pick a reasonable default too
- the collection frequency will automatically adjust based on the live size of the heap
    - track the total number of allocated bytes of managed memory
    - when it reaches a threshold we trigger a garbage collection
    - we then notice how many bytes were *not* freed and adjust the threshold accordingly
    - as the amount of live memory increases, we collect less frequently

## Garbage collection bugs

- in theory we're done, but GC bugs are gross
    - the collector must free dead objects and preserve live ones
- how is it possible for the VM to use an object later that the GC itself can't see?
    - through a pointer in some local variable on the C stack
    - many GC can look in the C stack, such as
    the **Boehm Demers-Weiser garbage collector**
- we would intentionally push an object onto the VM's value stack
to keep it alive, because the code between pushing and popping that could cause
a garbage collection and we needed that object alive
- but Nystrom left some bugs in for fun!

### Adding to the constant table

- the constant table each chunk owns is a dynamic array
- when adding a constant to the function's table, that array may need to grow
    - but the constant being added may be a heap-allocated object
- the new object being added is passed to `addConstant()`
    - the object can only be found in the parameter to that function on the C stack
    - if there's not enough space, the table will call reallocate
    which may trigger a garbage collection
    - then this object is swept away before it can be added to the table
- solution: push the constant onto the stack, temporarily

### Interning strings

- all strings are interned in clox, so when a string is made, it is added to the intern table
    - if the string is new it can't be reached anywhere...and resizing the string pool
    can trigger a collection

### Concatenating strings

- for strings, we pop two operands, compute result, and push new value
    - but we must allocate space for a new array on the heap $\rightarrow$ garbage collection
    - we thus peek them off the stack instead of popping, so they stick around
    - we safely pop them once the resultant string is made
- in general, finding bugs is hard here, because you are looking for an object
that *should* be there but *isn't*

## Design note: generational collectors

- a collector loses throughput if it spends a long time re-visiting objects that are alive,
but it can increase in latency if it avoids collecting and accumulates a large pile of garbage
    - would be fixed if we could tell which objects were likely to live long
- there is a method to doing this
- **generational hypothesis** or **infant ortality**: most objects are very short-lived
but once they survive beyond a certain age, they stick around for a long time
    - the longer an object *has* lived, the longer it likely will *continue to live*
- resulted in a new technique called **generational garbage collection**
    - every object that gets allocated goes into a small region of heap called "nursery"
    - the GC is invoked frequently over the nursery region
- each time the GC runs over the nursery, we increase a "generation"
    - anything that survives is considered a generation older, which the GC tracks
    - if an object survives a number of generations (often, just one), it
    becomes *tenured* and copied into a much larger heap region for long-lived objects
    - this region is also garbage collected bar far more infrequently

## Exercises

1. The Obj header struct now has three fields: `type`, `isMarked`, and `next`.
How much memory does these take up? Can you come up with something more compact?
Is there a runtime cost to doing so?

2. When the sweep phase traverses a live object, it clears the `isMarked` field
to prepare for the next collection cycle. Can you come up with a more efficient approach?

3. Mark-sweep is only one flavor of garbage collection. 
Replace or augment the current collector with another one. 
Consider reference counting, Cheney's algorithm, or the Lisp 2 mark-compact algorithm.
