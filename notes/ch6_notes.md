chapter 6 notes:
- goal: turn sequence of tokens into a syntax tree
- some grammars are **ambiguous**, as in there are 
multiple parsings that can lead to that string
    - this is bad.
    - develop rules to fix this:
- **precedence**: which opreators are evaluated first with multiple different operators
- **associativity**: which operators are evaluated first in a series of the same operator
    - *left associative* operators are evaluated *left-to-right*
        - e.g. `-`
    - *right associative* operators are evaluated *right-to-left*
        - e.g. exponentiation or assignment
    - *non-associative* operators cannot be used together
- the precedence/associativity that we will use:
    - equality (`==`, `!=`)
    - comparison (`>`, `<`, `>=`, `<=`) 
    - term (`-`, `+`)
    - factor (`/`, `*`)
    - unary (`!`, `-`)
    - all of these are left associative except for unary operators
    - our lowest precedence is any number, string, `true`, `false`, `nil`,
    or `"(" expression ")"` (parentheses)
- use this difference in precedence as a different rule in the grammar
    - rules can match expressions at its precedence level or hight, but not lower
- consider the following rule:
    `factor -> factor ("/" | "*") unary | unary;`.
    - this makes factors left associative (good),
    but since the first terminal is the same as the head of the rule,
    this is a **left-recursive** production
    - many parsing techniques do not like left-recursion
- consider the same rule, done as the following:
    `factor -> unary ( ("/" | "*") unary )*;`
    - this allows `factor` to be a `unary` expression
    followed by a sequence of mults/divs
- one way to parse: **recursive descent**
    - a **top-down parser** since starts from the outermost grammar rule (`expression`)
    and works down into subexpressions to reach leaves
- translation of grammar rules into imperative code:
    - terminal: code to match/consume token
    - nonterminal: call to rule's function
    - `|` : `if` or `switch`
    - `*` or `+`: `while` or `for`
    - `?` : `if` block
- each method for parsing a grammar rule produces a syntax tree
    for that rule and returns it to the caller
    - a reference to another rule calls that rule's method
- **predictive parser**: a parser which looks ahead to decide how to parse
- a parser's jobs:
    - given a valid sequence of tokens, produce a 
    corresponding syntax tree
    - given an invalid sequence of tokens, detect any errors
    and tell the user about their mistakes
- what to do with a syntax error:
    - detect and report the error
    - avoid crashing or hanging
    - be fast
    report as many distinct errors as there are
    - minimize cascaded errors
        - 'phantom' errors

Synchronization and error handling
- **error recovery**: the way a parser responds to an error
and keeps going to look for later errors
    - **panic mode**: a recovery technique where the
    parser immediately enters a 'panic mode' whenever
    there is any sort of error and tries to get its
    state/sequence of tokens aligned so it is back on track
        - this is called **synchronization**
    - occurs by selecting some rule in the grammar,
    jumps out of any nested productions until it
    returns to that rule, and discards tokens
    until it reaches one that can appear at that
    point in the rule
    - synchronizing ought to happen between statements
- **error production**: a grammar rule that is supposed to match
*bad* syntax
- in recursive descent, the parser's state is not 
explicitly stored, but known via the call stack
    - therefore, when in the middle of being parsed,
    and there's an exception, we need to clear the
    bad frames
- therefore, we will throw the error when desired
and *catch* it higher up in the call stack
    - then we must discard tokens until the next statement (the semicolon)

## Additions
- I added both the comma operator and the ternary operator.
    - note: the author of this book was stupid and I literally could not
    fix the argument thing for functions because we haven't gotten there yet.
    So **remember** to come back and fix that.
    - I had to add a whole new expression type for the ternary operator.
    Hopefully that doesn't screw me over later?
