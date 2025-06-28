# Ch. 12 : Classes

- time to implement object-oriented programming (ew)

## OOP and Classes

- three ways to approach object-oriented programming:
    - classes: the one I know and love...
    - prototypes: JS and Lua
    - multimethods: CLOS, Julia
- classes:
    - bundle data with code that acts on it
    - has a *constructor* to create and initialize an *instance* of the class
    - has a way to store and access *fields* on the these instances
    - has *methods* shared by all instances that operate on each instance's state
    - inheritance (next chapter)

## Class declaration

- a new name via `class` statement, so new grammar rules
    - basically, a class is a set of functions encapsulated by a class declaration
    - there is no `fun` keyword, though...it's just the name
    ```
    class Breakfast {
        cook() {
            ...
        }

        serve(who) {
            ...
        }
    }
    ```
- as Lox is dynamically typed, fields are not explictly listed in the class declaration
    - you can freely add or remove fields as desired

## Creating Instances

- classes don't do anything yet, and it doesn't have `static` methods, so classes are useless without instances
- way to create instances is not standard
    - `new` keyword in C++ and Java
    - Python calling the class itself
    - some have callign a method in the object
- Lox will do the Python approach

## Properties on Instances

- either add behavior first (methods) or state first (properties)
    - properties first because they get quite entangled
- every instance is an open collection of named values
    - methods can access and modify these properties
    - so can outside code
    - accessed via the `.`: `object.property`
> note: allowing code outside the class to modify an object's fields goes against the principle that classes *encapsulate* state
- properties are resolved at runtime
    - thus, they are not resolved by the resolver
    - they are stored in a hash table
    > this is fast for most languages, but not ideal. 
    > Most dynamic language optimizations rely on static techniques in fact
- what if a property doesn't have a property with a given name?
    - this could be `nil` but it's better to make it an error because that's usually unintended
- note: **fields** are named bits of state stored directly in an instance
    - **propeties** are named *things* that a get expression may return
    - every field is a property, but not all properties are fields

### Set expressions

- now it's time to be able to set a property
    - just extend the assignment rule
    - note that in something like
    ``` 
    object.prop1.prop2.prop3 = 4;
    ```
    - `prop1`, `prop2` are getters, and `prop3` is being set
- remember that assignment is handled strangely; we don't know it's an assignment
until we get to the `=`
    - with a `call` on the lefthand side, the final `=` might be far far away
    - so, we parse the left-hand side as a normal expression
    - then, if we find an equal sign, then we parse and transform it to the correct syntax node
    
## Methods on Classes

- parser already deals with method calls (let's go)
    - don't need the parser to work on method calls, either (let's go)
- consider the following:
```
var m = object.method;
m(argument);
```
- supposing `method` is a method and not a field, what should this do?
    - stores result of a method? in a variable? And then calls it?
    - can the method be considered a function?
- now consider this?
```
class Box {}

fun notMethod(argument) {
    print "called function with " + argument;
}

var box = Box();
box.function = notMethod;
box.function("argument");
```
- program creates an instance, stores a function in a field, and calsl it using syntax of a method call
    - for Lox, these will both work
    - calling a function stored in a field is apparently useful (first-class function)
- first example also useful because hoisting a asubexpression out into a local var without changing meaning
    - if you expect
    ```
    var part1 = instance.method(var);
    function(part1, "hello");
    ```
    to work, and `()` and `.` are separate expressions, you should be able to *hoist* the lookup part into a variable
- in this example:
```
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
does `method` here know the instance from which it was accessed?
    - what about this continuation:
    ```
    var bill = Person();
    bill.name = "Bill";
    
    bill.sayName = jane.sayName;
    bill.sayName();
    ```
    This feels wrong on every level.
- Nystrom has deemed that Lox will have methods "bind" `this` to the original instance when the method is accessed
    - Python calls these **bound methods**
- note that in this implementation, the fields are attempted to be accessed before methods
    - this indicates that *fields shadow methods*, i.e. naming a field the same name as a method overwrites the method, so to speak

## This

- behavior and state are not tied together quite yet
    - i.e. methods cannot access fields yet
- need a way to access the fields of the "current" object
    - the `self`, `this` of the programming language
- since methods are accessed and then invoked, `this` will refer to the object from which the method was accessed
- Consider:
```
class Egotist() {
    speak() {
        print this;
    }
}

var method = Egotist().speak;
method();
```
In this snippet, we get a reference to `speak()` method off an instance of the class
    - this method must *remember* that instance so it can find it when the function is called
    - need `this` at the *moment* the method is accessed and attach it to the function...
    - this is just another *closure*
- thus, we defined `this` as a "hidden variable" in the environment that surrounds the function returned
    - then, looking up `this` would just be looking up that variable
- check this out:
```
class Cake {
    taste() {
        var adjective = "delicious";
        print "The " + this.flavor + " cake is " + adjective + "!";
    }
}

var cake = Cake();
cake.flavor = "German chocolate";
cake.taste();
```
In evaluating the class definition, we get a `LoxFunction` of `taste`
    - it's closure is the environment surrounding the class, which is global
        - classes do not have their own closures, check `Stmt.java`
    - in evaluating `cake.taste` get expression, we create a *new environment* that binds `this` to the object the method is accessed from
        - in this case, `cake`
    - then, we make a *new* `LoxFunction` with the same code as the original but with that new environment as its closure
        - this is the function that is returned when evaluating the get method
    - that function being called works exactly as normal and gets its own environment
- this covers weird use cases like:
```
class Thing {
    getCallback() {
        fun localFunction() {
            print this;
        }

        return localFunction;
    }
}

var callback = Thing.getCallback();
callback();
```
This should work properly.
- we add `this` as a local variable in a new scope for the class
    - there needs to be a corresponding *environment* to match the chain of scopes
    - as this variable is only accessible inside the class, we create the environment at runtime after we find the method on the instance

### Invalid uses of this

- we should not allow calling `this` outside of a method
    - therefore there is no instance for `this` to point to if you're not in a method
- i.e. this should just be an error, statically
