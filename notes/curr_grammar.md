program -> declaration* EOF;
declaration -> varDecl | statement;
varDecl -> "var" IDENTIFIER ("=" expression)? ";" ;
statement -> exprStmt | printStmt | block | ifStmt | whileStmt | forStmt;
whileStmt -> "while" "(" expression ")" statement;
forStmt -> "for" "(" (varDecl | exprStmt | ";") expression? ";" expression? ")" statement;
block -> "{" declaration* "}" ;
exprStmt -> expression ";" ;
printStmt -> "print" expression ";" ;
ifStmt -> "if" "(" expression ")" statement ("else" statement)? ;
expression     → comma;
comma -> assign_or_condition ("," assign_or_condition)* ;
assign_or_condition -> IDENTIFIER "=" assign_or_condition | logic_or ("?" expression ":" assign_or_condition) ;
logic_or -> logic_and ("or" logic_and)* ;
logic_and -> logic_xor ("and" logic_xor)* ;
logic_xor -> equality ("xor" equality)*;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary
               | ("++" | "--") IDENTIFIER ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" | IDENTIFIER ;