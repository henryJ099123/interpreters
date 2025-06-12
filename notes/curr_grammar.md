program -> declaration* EOF;
declaration -> varDecl | statement;
varDecl -> "var" IDENTIFIER ("=" expression)? ";" ;
statement -> exprStmt | printStmt | block ;
block -> "{" declaration* "}" ;
exprStmt -> expression ";" ;
printStmt -> "print" expression ";" ;
expression     → comma;
comma -> assign_or_condition ("," assign_or_condition)* ;
assign_or_condition -> IDENTIFIER "=" assign_or_condition | equality ("?" expression ":" assign_or_condition) ;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary
               | primary ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" | IDENTIFIER ;