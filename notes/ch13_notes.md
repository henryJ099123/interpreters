# Ch 13: Inheritance

## Superclasses and Subclasses

- Simula in making classes used *subclass* to refer to that
    - *superclass* was added later
- for declaration, we use `<` to indicate the inheritance in code
- option for *no* superclass
    - unlike Java, which always has a root class
- store the superclass as a variable expression, rather than a token, because
at runtime the identifier is evaluated as a variable access
    - it's for the resolver

## Inheriting methods

- inheritance implies almost all of the superclass ought to be true of the subclass
    - a lot of implications in statically typed languages: sub*types*
        - think polymorphism
- however all this really means is that methods can be called up the chain

## Calling superclass methods

- we may also want to **override** a superclass method, which is already easy by redefining
    - however, usually a subclass wants to *refine* the method, not replace it
    - i.e. we need a `super` or equivalent 

### Syntax

- yikes new token (`super`)
    - cannot have `super` by itself
    ```
    print super; // syntax error
    ``` 
- a super call is simply a super *access* followed by a good old function call

### Semantics

- which superclass does `super` start looking things up from?
    - naive answer: of `this`, which is usually coincidentally correct, but not always:
```
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
- this ought to print "A method"
    - however, `this` refers to C here, and thus `super` will refer to B, which is bad
- lookup ought to start on the superclass *of the class containing the `super` expression*
    - call `test` on C
    - enters `test` from B
    - superclass of B is A, so chains onto `method` in A
- so, in order to evaluate a `super` expression, we need the superclass of the class definition surrounding that call
    - we *could* add a field to LoxFunction to store a reference to a LoxClass that might hold the function but that's a lot of plumbing
    - why don't we just use closures again?
- in this case, we need to bind the information at class *declaration*, as it is *not* a property at runtime
    - the `super` always refers to the same superclass
- so, we create a new environment for the superclass once at class definition to bind the superclass to `super`
    - creating a LoxFunction will capture this environment in its closure
- look up surrounding class's superclass via "super" in the proper environment
- when accessing a method, we need to bind `this` to the object the method is accessed from
    - however, the current object is *implicitly* the same one we've been using, i.e. `this`
    - the *method* is on the superclass; the *instance* is still `this`

### Invalid uses of super

- what if you use `super` without a asuperclass?
