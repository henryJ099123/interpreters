# Ch. 27: Classes and instances

- object oriented programming is a *bunch* of intertwined features:
    - classes, instances, fields, methods, initializers, and inheritance
- there's really a gradient to object-oriented design:
    - Self has objects without classes
    - CLOS has methods but without being attached to specific classes
    - C++ did not have runtime polymorphism or virtual methods
    - Python has multiple inheritance but other languages do not
    - Ruby allows defining methods on a single object

## Class objects

- in a class-based OOP language, everything starts with classes
- treat classes as another object type to the VM

## Class declarations

- could have made class declarations to be *expressions* explicitly bound to a variable
    - like a lambda function, but for classes
    - i.e. like

    ```text
    var Pie - class {}
    ```

- the instruction for creating the class takes a constant table index of the class's name
- nothing in the body of the class (yet) because no methods (yet)
- Lox allows "local classes"
    - the string for the class's name from the constant table is loaded
    from the constant table
    - then a class is pushed onto the stack
    - that's either put into a global variable or left on the stack as a local
- in essence, what that function ought to do is:
    - put the class in the constants table
    - put bytecode to define a new class at runtime with index to it in the constants table
        - this will put the class on the stack
    - put bytecode for a new global variable or local variable as needed

## Instances of classes

- classes serve two main purposes:
    - **create new instances**
    - **contain methods** that define how instances of the class behave
- Lox lets users freely add fields to an instance at runtime
    - hash table best for this
    - this is most often the biggest practical difference bteween static and dynamic languages
        - results in a large speed difference
- Lox *language*'s notion of "type" and the VM's understanding of *type* are different
    - in the C code, each instance of a class is the same "type", an ObjInstance
    - to a Lox *user*, the instances of different classes are different types
- most object-oriented languages define a `toString()` method that lets
the class specify how its instances are converted to a string
    - this would be good support
- new instances are made by calling the class

## Get and set expressions

- we just need to expose state setting to the user
- Lox uses the "dot" syntax for accessing or setting these fields
    - it works as an infix operator
- **property** is general term for named entity accessed on an instance
    - **fields** are the subset of propreties that are backed by the instance's state
- need to use the `canAssign` boolean argument to prevent something like
```text
a + b.c = 3
```
from compiling properly
- we also need to check we are "getting" from something actually an ObjInstance
    - nothing stopping from writing something like:

    ```text
    var obj = "not an instance.";
    print obj.field;
    ```

- we *could* suport fields on values of other types, but this is a bad idea
    - it complicates the implementation quite a bit, and raises bad questions
    about equality and identity


## Exercises

1. Trying to access a non-existent field on an object immediately aborts the entire VM.
The user has no way to recover from this runtime error, nor is there any way
to see if a field exists *before* trying to access it.
It is the user's prerogative to read only valid fields.

How do other dynamically typed languages handle this?
What should Lox do?
Implement your solution.
    - Python makes it an error as well, but you can recover
    from any error in Python via a `try...except` block
    - I'm not familiar with other dynamic languages
    - I think it should remain an error: force the user to program better,
    but perhaps having a way to `try...catch` runtime errors would be nice
    - perhaps I *could* implement this?
        - this would be very useful
    - this would need reworking how error handling is done at all in Lox

2. Fields are accessed at runtime by the *string* name.
But the name must always appear directly in the source code as an
*identifier token*. A user program cannot `imperatively` build a string value
and then use that as the name of a field. Should they?
Devise a language feature that enables it and implement it.

3. Lox has no way to *remove* a field from an instance.
You can set it to `nil`, but the entry in the hash table is still there.
How do other languages handle this?
Choose and implement your strategy for Lox.

4. Because fields are accessed by name at runtime,
working with instance state is slow.
It's constant time, but those constant factors are large.
This is a major component of why dynamic languages are slower than statically typed ones.
How do sophisticated implementations of dynamically typed languages
cope with this and optimize it?
