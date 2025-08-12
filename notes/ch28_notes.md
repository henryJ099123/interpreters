# Ch. 28: Methods and initializers

## Method declarations

### Representing methods

- use a table to store methods, same thing in jlox
    - problem is how the bytecode instructions can give the
    same information when the interpreter used to have access
    to the entire abstract syntax tree node

### Compiling method declarations

- compiling a class declaration tricky because the class can compile any number of methods
    - so, bytecode for a class declaration is split into a series of instructions
    - compiler already emits an `OP_CLASS` instruction that creates a new empty ObjClass object
- after that, emit new `OP_METHOD` instruction that adds a single method to the class
    - to the user, a class declaration is a single atomic operation
    - to the VM, it's a series of mutations on an object

### Executing method declarations

- the VM trusts the instructions it executes are valid because it only gets code from its
own compiler
    - many bytecode VMs support executing separately-compiled bytecode
    - the JVM does a check for valid bytecode
    - CPython does not, and chalks it up to the user

## Method references

- most times, methods are accessed and immediately called:

```text
instance.method(argument);
```

But sometimes, we can separate access and call:

```text
var closure = instance.method;
closure(argument);
```

As users can separate these processes they must be implemented separately.
- obvious solution is to look up the method in the class's table
and return the ObjClosure associated with that name,
but we also need to bound `this` to the instance the method was accessed from

```text
class Person {
    sayName() {
        print this.name;
    } 
} 

var jane = Person();
jane.name = "Jane";

var method = jane.sayName;
method();
```

In jlox we solved this via the existing Environment class, which is heap-allocated.
    - in clox, we have locals/temporaries on the stack, globals in a table
    (in my case in an array), and variables in closures use upvalues
    - more complex architecture $\Rightarrow$ more complex solution

### Bound methods

- when the users execute a method access, we find the closure for that method
and wrap it in a new "bound method" object that tracts the instance that the method
was accessed from
    - this will help wire up `this` to point to the instance

### Calling methods

- now have to implement actually *calling* the methods

## This

- we treat `this` as a lexically scoped local variable whose value
is initialized to the instance calling the method
    - as it's a local variable, a lot is already done for us
- currently, the lookup for this variable will fail because `this` is never 
declared explicitly by the user
    - until captured by closures, each local variable is on the VM's stack
    - the compiler sets aside stack slot zero by declaring a local variable whose name
    is an empty string
    - for functions, this slot holds the function being called
        - function never accesses this slot because it has no name
    - for *method* calls we let this slot hold the instance and have `this` point to it

### Misusing this

- also need to account for when `this` is declared outside of a method
    - top level script or function
    - we need to be aware of a function nested in a method, too
- solution: add information about the closest surrounding class

## Instance initializers

- objects should always be in a valid, meaningful state
- initializers work like normal methods with a few tweaks:
    - runtime automatically invokes the initializer method whenever an instance of a class is made
    - caller that constructs an instance always gets the instance back after the
    initializer finishes, regardless of what the initializer function returns
        - the initializer method does *not* need to explicitly return `this`
    - an initializer is *prohibited* from returning any value at all

### Initializer return values

- need to mandate that `init` returns the new instance
- done via a new `FunctionType` enum value

### Incorrect returns in initializers

- easy because of new functionType

## Optimized invocations

- method calls in clox are still slow
- Lox defines a method invocation as *two operations*: access and call
    - these can be separated by the user, so we have to support that
    - however, *always* executing those as separate has a significant cost
- every time a Lox program accesses and invokes a method,
it's wrapped in an ObjBoundMethod and then immediately accesses them again
    - then the GC has to spend time freeing all of that stuff
- most of the time, a Lox program should just access a method
and immediately call it
    - it's so immediate that the compiler can see it happening
- often, a bytecode VM will execute the same series of instructions one after the other
    - a classic optimization is to define a new single instruction,
    a **superinstruction**, to do all those little thingsin a single instruction
    - decode and dispatch is the largest performance drain of a bytecode interpreter
    - the challenge: *which* instructions are common enough to benefit from this optimization
- according to Nystrom this is about 7 times faster on his machine

### Invoking fields

- small problem now: if we have a `(` following something after a `.`, we
compile it as a method invocation, even if it is actually a *field* being called
- i.e. this breaks:
```text
class Oops {
    init() {
        fun f() {
            print "not a method";
        } 

        this.field = f;
    } 
} 

var oops = Oops();
oops.field();
```
- sometimes the user wants a faster answer even if it sometimes is wrong,
called **Monte Carlo algorithms**, but we can't unilaterally sacrifice correctness for speed
because the user hasn't dictated that
- typical optimization pattern:
    1. recognize common operation/sequence that is performance critical
    2. add optimized implementation
    3. guard optimized code with some conditional logic that validates the pattern applies

## Design note: novelty budget

- possibility of designing a language that looks and behaves however we want
    - doing different things for difference's sake
    - Nystrom recommends doing this; more art languages, fun, avant-garde ones
- but, if the goal is "a large user-base", you want to have the language
be loaded into many brains...which is hard
    - moving a new syntax and semantics
    - don't want to waste time on a language that isn't useful
- natural approach: *simplicity*
    - small languages
    - dynamic over static
- however, simplicity can sacrifice power and expressiveness
- a user does not have to load the *whole* language into their heads,
just the part that wasn't already there: the *delta* between the two
    - i.e. *familiarity* can be key
- Nystrom calls this a **novelty budget** or a *strangeness budget*
    - this is a *low threshold* for most users
- adding new, weirdo stuff is needed to make a language compelling, but
you need to spend it wisely or else you alienate the language
    - ends up with reserved syntax and adventurous semantics

## Exercises

1. the hash table lookup to find the `init()` method is constant time, but slow.
Implement something faster.

2. In a dynamic language, a single callsite may invoke a variety of methods.
But most of the time, a callsite ends up being the exact same method on the exact same
class for the duration of the run. Most calls aren't actually polymorphic.
How do advanced implementations optimize based on that?

3. The `OP_INVOKE` instruction has two hash table lookups. Looks for a field that shadows
a method, and then looks for the method. This first check is rarely useful.
Most fields don't contain functions. But it's *necessary* because the language allows it.
This is a language *choice*. Is it a good one? What would you have done differently?

