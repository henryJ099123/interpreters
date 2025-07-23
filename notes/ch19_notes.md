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

## Design Note: String encoding

- String encoding can be kind of...difficult to handle
- What do we call a single "character"?
    - C uses ASCII, but that gives no option for accents or funky character stuff
    - Unicode was next with tons of **code points** that continued to grow until it now includes things like emojis and whatnot, in 16 bits
        - handled some gylphs by **combining characters** (e.g. accents on subsequent characters)
    - if we combine characters...how does indexing work? Are they considered separate? United?
        - united, they're called an **extended grapheme cluster**
- How is a single unit represented in memory?
    - ASCII gets a single byte (and leaves the high bit unused)
    - Unicode has several common encodings
        - UTF-16 was good for putting things in 16bits, but when it overflowed, it led to *surrogate pairs* of 16 bits to represent a single code point
        - UTF-32 uses 32 bits for each "code point"
    - UTF-8 is apparently the most complex: it uses a variable number of bytes to encode a code point
        - this means it is not possible to directly index into the string, must walk it
- these differences are actively challenging. Python switching its encoding from Python 2 to 3 was emblematic of this
- pros and cons:
    - ASCII is memory efficient and fast, but if you don't use the English or latin alphabet, good luck
    - UTF-32 is fast but wastes a lot of memory
    - UTF-8 is memory efficient and supports the entire Unicode range but the variable-length encoding makes arbitrary indexing slow
    - UTF-16 is apparently the worst: less memory efficient but still variable-length encoding
- one option: support all the Unicode code points and internally select an encoding for each string based on its contents
    - really complex and hard to implement and debug
    - string serialization between systems gets really annoying
    - users need to understand the APIs and how to use which one
    - used in Swift and Raku
- compromise: always encode with UTF-8 and allow third-party stuff for other encodings
    - less Latin-centric and not more complex, but no fast direct indexing
- Nystrom would go with the maximal approach

## Exercises

1. Each string requires two separate dynamic allocations: one for the ObjString and second for the char array.
This also means accessing the actual string requires *two* pointer indirections (cache unhappy).
Use **flexible array members** to store the ObjString and its character array 
in a single contiguous allocation.
    - this doesn't seem *terribly* difficult...except for one small issue:
    - I don't believe there's an easy way to give away a pointer to a string
    - i.e. the `takeString` method just goes away
    - which it does! And now it is done
2. In creating the ObjString for each literal, the characters are copied onto the heap.
So, in freeing the string, we can free the characters too.
To save more memory, we could instead track which ObjStrings *own* their character array
and which are "constant strings" that just point back to their original source string
(or to some non-freeable location). Add support for this.
    - This...feels almost like...a waste of time?
    - It's just a bunch of small changes?
    - It doesn't even really work when you mix in number 1...which I've already done...
3. If Lox was your language, what would you have it do when using `+` with a string operand and some other type?
    - well, I think I'd just turn the other thing into a string and then append it.
    - C treats them as numbers and just performs arithmetic
    - Java does this
    - Python throws an error, you have to explicitly type cast
