# Ch. 18: Types of Values

- This chapter is a bit of respite
- Lox is dynamically typed, so any variable can hold any kind of thing
    - we have numbers now, but we will add Booleans and `nil` in this chapter
> Note: there's a third category next to statically typed and dynamically typed, known as *unityped*.
> You don't typically see this one anymore.

## Tagged Unions

- I knew this was coming!
- For our runtime representations of values, we have a few questions to ask:
    - **How do we represent the type of value?**
        - we need to know that a given data is actually a boolean, or a number, so we need to tell what it's type is
    - **How do we store the value itself?**
        - where do we actually put the value?
- We have to do this *efficiently* too, but the simplest and classic solution is a **tagged union**
    - has two parts: a type "tag" and a payload for the value
- we build an enum in `value.h` to hold the different values that have *built-in support* for the VM
    - user-defined classes do not need entries here, because the VM treats them all the same
- could define a struct for fields of each possible type to be our "value", but this wastes memory
    - a value can't be two things simultaneously
    - thus, we use a union, which lets its fields overlap in memory
    - size of union size of its largest field
- so, each Value is stored as a struct which holds the type as a field and the actual payload as a union as a field
    - union field has an 8-byte double, so there will be four bytes of padding after the type, which results in quite a bit of wasted space
        - especially because this type only has 4 values
- thus, each value is 16 bytes, which is large, because we effectively spend 8 bytes on the type tag
    - this will get better, but fine for now, especially because we pass around by value
    - these are **immutable**, i.e. you can't modify some underlying state about them

## Lox values and C values

- currently the rest of the code things Value is just `double`, so C casting won't work here
    - however, a Value can *contain* a double
    - macros to convert back and forth, and to check the type

## Dynamically typed numbers

- now we fix all old instances of value being treated just as a double
    - this leads to runtime errors
    - Lox literally just immediately halts the interpreter at a runtime error, and there's no way to recover from it
        - thus, this would be a thing to remedy
- for validation, we could just pop it first...but in later chapters, leaving it on the stack in general is better for the garbage collector
- the runtime error function is a *variadic function*, one that takes a varying number of arguments
    - the `...` and the `va_list` allows passing an arbitrary number of arguments
    - `vfprintf` is `printf` but that takes a `va_list`
    - basically allows formatting in the error

## Two new types!

- adding bools and `nil`
- number literals needed to deal with billions of possible numeric values
    - store the literal's value in the chunk's constant value and emit a bytecode instruction to load it
    - could store `true` in the constant table and use `OP_CONSTANT` to read it
- these are only three values though. why put in a two-byte instruction and a constant table entry for them?
    - define dedicated instructions to push these on the stack
    - the VM takes equal time reading and decoding, so fewer/simpler instructions needed, the better
    - Java bytecode has dedicated instructions for loading 0.0, 1.0, 2.0, etc.

### logical not and falsiness

- time to implement the boolean negation!

### Equality and comparison operators

- get the equality operators done too!
- we aren't including explicit instructions for `<=`, `!=`, and `>=`
    - the bytecode compiler need not closely follow the user's source code
    - just need right user-visible *behavior* (even if it would be faster here)
    - we can just say `a <= b` is just `!(a > b)`
> small issue. All comparison operators should return false if an operator is `NaN`.
> Unfortunately, `NaN <= 1` and `NaN > 1` are both false, but our desugaring does *not* do this.
- need to have a way to evaluate truthiness
    - javascript has implicit conversions of types
    - most dynamically typed languages that have an `int` and `float` type consider different things equal if their values are equal
- for comparison of truthiness between two things, we have to turn them to C things to evaluate similarity rather than `memcmp`
    - this is because of the padding, and we cannot know what the bits are in the padding

## Exercises

1. We can reduce the binary operators even further than before...how so?
    - can reduce to a single instruction that returns less, equal, or greater
    - can then define the other operations in terms of this one with some simple manipulation
2. How can you increase the number of opcodes to go faster?
    - actually include `OP_LESS_EQUAL` and such
    - add comparisons to 0, 1, etc.
