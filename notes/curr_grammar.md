program -> declaration* EOF;
declaration -> varDecl | funDecl | statement;
funDecl -> "fun" function;
function -> IDENTIFIER "(" parameters ")" block ;
parameters -> IDENTIFER ("," IDENTIFIER)* ;
varDecl -> "var" IDENTIFIER ("=" expression)? ";" ;
statement -> exprStmt | printStmt | block | ifStmt | whileStmt | forStmt | breakStmt | continueStmt | returnStmt;
returnStmt -> "return" expression? ";";
breakStmt -> "break;"
continueStmt -> "continue;"
whileStmt -> "while" "(" expression ")" statement "aftereach" statement;
forStmt -> "for" "(" (varDecl | exprStmt | ";") expression? ";" expression? ")" statement "aftereach" statement;
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
               | ("++" | "--") IDENTIFIER | call ;
call -> primary ("(" arguments? ")")* | IDENTIFIER "++";
arguments -> assign_or_condition ("," assign_or_condition)* ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" | IDENTIFIER | anonFun;
anonFun -> "fun" "(" parameters ")" block;