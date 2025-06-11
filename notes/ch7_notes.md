# Evaluating Expressions

- Now the interpreter will actually execute code
- jlox will just execute the syntax tree itself
by evaluating an expression to produce a value

## Representing Values

- In this dude's fake language, values are created
by literals, computed by expressions, and stored in variables (like most programming languages)
    - however, these are *implemented* in this
    interpreter language, so Lox's *dynamic typing* must be implemented in Java's *static typing*
    - sounds like a job for **polymorphism** to me
- have every Lox value be underneath a Java `java.lang.Object` type
    - see the below table:

| Lox Type | Java representation |
| -------- | :-----------------: |
| Any Lox value | Object |
| `nil` | `null` |
| Boolean | Boolean |
| number | Double |
| string | String |

- Given a static type Object, we find out *in runtime* what a Lox "type" is by using `instanceof`, a Java operator
    - aka we take advantage of Java's object representation to do everything
> Another nice thing we get to have: garbage collection.

## Evaluating Expressions

- need to implement evaluation logic
    - Have each syntax tree class have an `interpret()` method to essentially **interpret itself**
    (The *interpreter design pattern*)
    - Rather, will do the *Visitor pattern* again
- The `Interpreter` class *extends* the `Visitor` interface from before
    - so it can access all of the parts of each class without modifying those classes
        - I think modifying those classes would be more straightforward though...

### Evaluating Literals

- literals are leaves of the expression tree
    - a *bit of syntax* that produces a value; literally (pun intended) in the source code itself
    - *very close* to being a value itself
- before, we converted a *token* to a *tree node* in the parser, and now we convert the *tree node* into a *runtime value*

### Evaluating Parentheses

- we haven't yet "figured out" how to actually
evaluate what's inside of the parentheses, but
all we need to do is actually just evaluate it.
- So, recursively evaluate the subexpression by calling its accept method again

### Evaluating Unary Expressions

- Similar to the others; we evaluate the subexpression and then apply the operator
- for each operator type, it is *unknown* statically what kind of type the Lox expression is
    - therefore, there is a *cast* to Java's `double` primitive type
    - This *casting* is pretty much the core of what makes Lox **dynamically typed**
- This evaluation is a post-order-traversal: evaluate *children* before *parent*

### Truth and Falsehood

- what do we do when we use logical not (`!`) something that *isn't* a Boolean expression?
    - this could just be an error, but as most dynamically typed languages are very lax on implicit conversions, we should deal with it
- Typically, there are states of each type that are considered *true*-ish and *false*-ish
    - somewhat arbitrary
    - Nystrom has decided that `false` and `nil` are the only things considered *false*; everything else is considered *true*
    - i.e. an empty string `""` is considered *true*

### Evaluating Binary Expressions

- notice that in this binary expression we evaluated
the operands *left to right*
    - i.e. we did `evaluate(expr.left)` and *then* `evaluate(expr.right)`
    - be careful if expressions have side-effects! This difference can make all the difference
- for *overloaded operators*, such as the `+`,
we must **dynamically check** the type of the operands
to see what kind of thing we need to do
- recall: `==` in Java for non-primitive types will compare memory addr as they are pointers
    - use object's `.equals` method

## Runtime Errors

- now what happens when an expression is invalid?
    - casts can fail...
    - sometimes we let it happen (`C`) but this is *memory unsafe*
- **runtime errors**: errors deteced while code is executing, not during compiling
    - we want to *hide* that Lox is implemented in Java
    - we *do* want to stop executing any code when an error occurs
- we should have runtime errors *stop evaluating the expression* but **not** kill the interpreter

### Detecting Runtime Errors

- the tree-walk interpreter evaluates expressions recursively
    - so, if there's an error thrown, we have to unwind out far enough to catch it
- use Lox's own cast failure catching to handle it as desired
    - i.e. we check whether it is correct type, if not we throw an error
    - design our own error handler
- a subtle note: Lox *evaluates* both operands before checking *either* of their types

## Hooking up the interpreter

- Must create public API for the interpreter that Lox can use to actually evaluate its contents
- must deal with conversions between the different types
that implicitly exist between these languages

### Reporting runtime errors

- Error reporting in `Lox`; i.e. `Interpreter` throws the error, `Lox` catches it
    - note when we have a runtime error

### Running the interpreter

- `Lox` class uses a `static` variable for the `Interpreter` object because if the class runs successive lines we want the global variables to persist between them
