# Ch. 29: Superclasses

- Almost done!
- all that's left: inheriting methods and calling superclasses

## Inheriting methods

- Lox inheritance uses `<` as the `extends` delineator
- cannot inherit a class from itself, but there wouldn't actually be any issue
doing so in clox...just wouldn't be useful

### Executing inheritance

- In jlox, each class help a reference to its superclass
    - not great for performance, especially because it must happen at *invocation*
- copy all of the inherited class's methods down into the subclass's own method table
    - called "copy-down inheritance"
- this works in Lox because classes are *closed*: the set of methods cannot change
    - some languages like Python allow opening an existing classes and
    adding/removing methods
    - that would break this optimization because there's no way to pick up
    on these modifications after the subclass inherits from the superclass
    - open classes like these are found a bit dangerous by many and given
    the name "monkey patching" or "duck punching"
- what about method overrides?
    - those are compiled after the inheritance and so will override in the table

### Invalid superclasses

- what if you don't inherit from a class?

## Storing superclasses

- a class has no reference to its superclass right now
    - no good for super calls...
- remember this?
```text
class A {
    method() {
        print "A method";
    } 
} 

class B < A {
    method() {
        print "B method";
    } 

    test() {
        super.method();
    } 
} 

class C < B {}

C().test();
```
This *should* print `"A method"`, as `this` is an instance of C in the invocation at the bottom
    - super calls are resolved relative to the superclass of the surrounding class
    where the super call occurs
- this means that *super calls are static properties* and not determined at runtime
- in jlox, this superclass was stored in the same Environment structure
    - we will do something similar in clox with upvalues

### A superclass local variable

- creating a new lexical scope means that if we declare two classes in the same scope
each has a different local slot for the superclass
    - since the variable name is always `super` we need different scopes

## Super calls

- Lox doesn't really have super call expressions, more *access* expressions,
because `super` on its own doesn't really make sense...what would it be?
- for a program like follows:
```text
class Doughnut {
    cook() {
        print "Dunk this in the fryer.";
        this.finish("sprinkles");
    } 

    finish(ingredient) {
        print "Finish with " + ingredient;
    } 
} 

class Cruller < Doughnut {
    finish(ingredient) {
        super.finish("icing");
    } 
} 
```
the bytecode for the `super.finish("icing")` looks like this:
    - `OP_GET_LOCAL` (this) $\rightarrow$ cruller instance on top
    - `OP_GET_UPVALUE` (super) $\rightarrow$ doughnut class on top of cruller
    - `OP_GET_SUPER` (`finish`) context switch, puts function on top
    - `OP_CONSTANT` ("icing") put on top
    - `OP_CALL` eats the top two
- first instruction **loads the instance**
- second instruction **loads the superclass where the method is resolved**
- The new `OP_GET_SUPER` instruction encodes the **name of the method to access** as an operand

### Executing super access

- the code passes from compiler to runtime if the user didn't put `super` where it isn't allowed
- no need to check for fields because fields aren't inherited
- if Lox were a prototype-based language that uses *delegation* instead of *inheritance*,
we *would* need to check for inherited fields
    - delegation is when instances inherit from other instances

### Faster super calls

- this is slow, though, just like before, because of the allocations to an ObjBoundMethod
    - this is more likely for super calls too...no chance of looking up a field

## A complete virtual machine

- It's done! 2500 lines of C!
- implemented variables, functions, closures, classes, fields, methods, and inheritance
- portable to any C compiler for real-world production use
    - single-pass bytecode compiler
    - stack for storing varaibles without heap allocation
    - precise garbage collectors
    - compact object representations

## Exercises

1. A tenet of object-oriented programming is that a class should ensure new objects are
in a valid state. This means an initializer in Lox, `init`.
Inheritance does complicate invariants because the instance must be in a valid state
for *every* class in the inheritance chain.

The easy part: calling `super.init()` in each subclass's `init()` method.
The hard part: fields. Nothing stops two classes in a chain
from accidentally claiming the same field name, which will cause
stepping on each other's fields and leave with broken state.

How would you address this?

2. The copy-down inheritance optimization is valid only because Lox
does not permit modification of class methods after declaration.

How do languages who *do* allow this, like Ruby, keep things optimized?

3. In the jlox chapter on inheritance there was a challenge to implement a 
sort-of "reverse super". Do it here.

