Notes from chapter 4:
- need to split up the raw Lox source code into **tokens**
    - meaningful aspects of its grammar
- try to include error handling early on
- *lexeme*: a "token" or small sequence from plaintext
    - example: for the code `var language = "lox";`,
    the lexemes are `var`, `language`, `=`, `"lox"`, and `;`.
    - lexemes are *raw substrings* of the code; mixed with metadata,
    these are *tokens*
- when parser recognizes a lexeme, we immediately associate it with the *kind* of lexeme it is; see the `TokenType` enum in `TokenType.java`
    - if a lexeme represents a *literal*, i.e. a number or string
    the parser will convert this directly into a representation
    as a runtime object
    - at this point we also record the line number of the lexeme
    for error handling purposes
- parser is at heart a loop scanner. it reads a character, 
    figures out what lexeme the character belongs to,
    consumes it and following characters, and finally emits a token
- could use a regular expression to identify things: `[a-zA-z][a-zA-Z_0-9]*` is a regex for var names, for ex.
- **lexical grammar**: rules that determine how a particular 
language groups characters into lexemes
    - rules of the grammar are simple enough to be classified as a 
    **regular language** (I don't think this is true...in class we
    showed that C is not even context-free)
- **lookahead**: how far ahead the parser can peek without consuming
    - most language parsers use only one or two characters of lookahead
- as for *reserved words*, we could just do the same `case` business as the operators and literals
    - however, things get strange if the programmer uses a reserved word as a susbtring of a variable name
    - **maximal munch**: if two lexical grammar rules both match some text, the rule which matches the *most characters* wins
        - for example, `>=` is interpreted as `GREATER-EQUAL` instead of `GREATER` and `EQUAL`
    - can only detect a reserced word until the end of an identifier has been reached
    - reserved words are just reserved identifiers

