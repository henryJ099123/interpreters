program -> declaration* EOF;
declaration -> varDecl | statement;
varDecl -> "var" IDENTIFIER ("=" expression)? ";" ;
statement -> exprStmt | printStmt ;
exprStmt -> expression ";" ;
printStmt -> "print" expression ";" ;
expression     → comma;

comma -> assign_or_condition ("," assign_or_condition)* ;

assign_or_condition -> IDENTIFIER "=" assign_or_condition | equality ("?" expression ":" assign_or_condition) ;

assignment -> IDENTIFIER "=" assignment | conditional;
conditional -> equality ("?" expression ":" conditional) ;

equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary
               | primary ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" | IDENTIFIER ;