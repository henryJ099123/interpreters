# Ch. 19: Strings

- the VM can represent numbers, Booleans, and `nil`
    - all immmutable and small
- strings have no max length, and no single value can hold even a capped string length
    - Pascal actually had a max string length of 255 from a single byte
- therefore, we must use dynamic allocation

## Values and Objects

- two level representation: heap for large/variable-sized values, stack for atomic ones
- every Lox value can be stored in a variable or be returned from an expression
    - for smaller things, the payload is directly in the Value struct, in the `union`
    - larger things on the heap, and the Value's payload is a pointer to the memory
    - each type has its own unique data but also collective state so the garbage collector can manage them
- Use a collective "Obj" type to refer to these

## Struct inheritance

- while every heap-allocated value is an object, not all Obj are the same $\rightarrow$ need some sort of inheritance
    - no ability for `union` because sizes of each thing is totally variable
- **struct inheritance** is sort of like a tagged union where each Obj starts with a tag field that identifies the kind of object it is and then payload fields
    - each type is instead its own separate struct...but how to treat these uniformly?
- each Obj "subtype" needs the state of all Objs, so its first field is a generic Obj
    - if a struct is declared first in another struct, it is arranged in memory in its outer struct first and expanded in place
    - this means we can *take a pointer to a struct* and safely convert it *to a pointer to its first field*, and vice-versa
        - this is how "inheritance" is mimicked in C

## Strings

- these fancy tricks are how strings are represented in C code
- string escape sequences would be translated in the `string()` function
- why can't the ObjString simply point back to the source string?
    - some ObjStrings need to be created dynamically at runtime, like string concatenation
    - these need to be freed too, but we *can't* free something that points to the original source code
    - so, every ObjString has its own char array and can free it when needed
- note that in memory allocation for an Obj, we allocate for its "subtype", and fill in those fields when needed

## Operations on strings

- notice that for the case of
```
"string" == "string"
```
the user expects equality, even if the underlying objects exist separately

### Concatenation

- full languages provide much more string operations, but we only do concatenation in Lox
    - as Lox is dynamic, we won't know whether we want addition or concatentation with `+` since the type's are not explicit
    - thus, this is a runtime behavior
    - here is where we would add special concatenation stuff for each type to go with strings
- make sure to keep track of pointer ownership and heap stuff

## Freeing objects

- lastly, everyone's favorite, freeing!
- consider the following:
```
"st" + "ri" + "ng"
```
This will create objects for `"st"`, then `"ri"`, then `"stri"`, then `"ng"`, and finally `"string"`
    - results in a memory leak for `"stri"`
- The *Lox program* can easily forget about intermediate strings, but the VM has to deal with it
- full solution is a garbage collector
    - adding garbage collectors later into a language is very hard, apparently
    - the collector has to be able to find *every* bit of memory still being used, so live data isn't being collected
- goal is to avoid leaking memory right now
    - VM must hold references even if the program doesn't care
- method: create a linked list for every Obj
    - the VM can traverse the list to find every object that's been heap-allocated and whether the program or stack has a reference to it
- we *could* define a separate linked list node struct, but that requires allocation too, so we do an **instrusive list**, which means the `Obj` struct is a linked list node itself
- we use `reallocate` excessively because it helps the VM track how much memory is still being used
- this means the program will free everything when it needs to at the end, but nothing dynamically in the meantime


