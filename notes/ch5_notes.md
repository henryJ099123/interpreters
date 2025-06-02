chapter 5 notes:
- goal: representing the code (parse trees!)
    - do a post-order traversal on the parse tree
- **context-free grammars** (did in theory)
    - this stuff is literally what I just learned
    - recall *derivations*, rules, *terminals*, *nonterminals*, etc.
    - rules like `breakfast -> protein | bread | "eggs"`
    - naturally recursive
    - some differences: allow `()` within for grouping, `*` as Kleene star
    - so kind of expanding the rules with regular expression syntaxes
- need to design a grammar for `lox` to handle:
    - literals
    - unary expressions
    - binary expressions
    - parentheses
- will represent grammar using subclasses as the method for a tree structure
    - these are dumb classes (just glorified structs)
- as this is a rather rote file, will be generated with another file (`GenerateAst.java`)
- with these syntax trees, the interpeter will need to act differently with each one
    - i.e. the interpreter must select different code to handle each expression type
- expression problem: difficulty of adding both new 
'types' of expressions and 'operations' on those 
expressions
- **visitor pattern**: a way to approximate functional styles in OOP
    - involves creating a `Visitor` interface with a 
    new `visit` function for each extended thing
    - every function/operation on those things is a 
    new class that implements the interface
    - the original abstract class from which each 
    thing extends gets a general `accept` function
        - each of the extended subclasses must implement this function
    - the implementation of this `accept` function 
    involves having the passed `visitor` call 
    `visit` on `this` thing
- in our case, the `accept()` method will return a 
generic thing based on each implementation
