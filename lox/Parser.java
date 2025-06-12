package lox;

import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;

import static lox.TokenType.*;

class Parser {
    // special type of error (just a runtime exception)
    private static class ParseError extends RuntimeException {}

    // read tokens instead of characters
    private final List<Token> tokens;
    private int current = 0;
    
    // constructor
    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    // the initial statement!
    List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<>();
        while(!isAtEnd()) {
            statements.add(declaration());
        }

        return statements;

        // try {
        //     return expression();
        // } catch(ParseError error) { // why do anything else if an error is found!
        //     return null;
        // }
    }

    // nonterminal rules translate to function code
    private Expr expression() {
        return comma();
    }

    private Stmt declaration() {
        try {
            if(match(VAR))
                return varDeclaration();
            return statement();
        } catch(ParseError error) {
            synchronize();
            return null;
        }
    }

    private Stmt varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name");
        Expr initializer = null;

        // allows declaring variable without initializing it
        if(match(EQUAL))
            initializer = expression();

        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }

    private Stmt statement() {
        if(match(PRINT)) return printStatement();
        if(match(LEFT_BRACE)) return new Stmt.Block(block());
        if(match(IF)) return ifStatement();
        if(match(WHILE)) return whileStatement();
        if(match(FOR)) return forStatement();
        return expressionStatement();
    }

    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    private Stmt ifStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'if'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after condition of 'if'.");
        Stmt ifTrue = statement();
        Stmt ifFalse = null;
        if(match(ELSE)) {
            ifFalse = statement();
        }
        return new Stmt.If(condition, ifTrue, ifFalse);
    }

    private Stmt whileStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'while'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after condition of 'while'.");
        Stmt body = statement();
        return new Stmt.While(condition, body);
    }

    private Stmt forStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'for'.");
        Stmt initializer;

        if(match(SEMICOLON)) initializer = null;
        else if(match(VAR)) initializer = varDeclaration();
        else initializer = expressionStatement();

        // a "blank" condition means true for a for loop
        Expr condition = new Expr.Literal(true);
        if(!match(SEMICOLON))
            condition = expression();
        consume(SEMICOLON, "Expect ';' after condition.");

        Expr increment = null;
        if(!match(SEMICOLON))
            increment = expression();
        consume(RIGHT_PAREN, "Expect ')' after increment.");

        Stmt body = statement();

        // throw increment at end if it's there
        if(increment != null)
            body = new Stmt.Block(Arrays.asList(body, new Stmt.Expression(increment)));
        // create while loop with the condition
        body = new Stmt.While(condition, body);
        // initialize before the loop
        if(initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }

        return body;
    }

    private Stmt expressionStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Expression(value);
    }

    // the reason this returns a list of statements
    // rather than a Block statement is because this code
    // will be reused for function bodies
    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<>();
        while(!check(RIGHT_BRACE) && !isAtEnd())
            statements.add(declaration());
        consume(RIGHT_BRACE, "Expect '}' after block.");
        return statements;
    }

    private Expr comma() {
        Expr expr = assign_or_condition();

        while(match(COMMA)) {
            Token operator = previous();
            Expr right = assign_or_condition();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // since I'm making these have equal precedence
    private Expr assign_or_condition() {
        Expr expr = logic_or();
        if(match(EQUAL)) {
            Token equals = previous();
            Expr value = assign_or_condition();
            
            // if its a variable it'll boil down to an Expr.Variable
            // then we can extract its name token and return the correct assignment expression
            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable) expr).name;
                return new Expr.Assign(name, value);
            }

            error(equals, "Invalid assignment to left-hand side.");

        } else if(match(QUESTION)) {
            Expr left = expression();
            consume(COLON, "Expect ':' after the 'ifTrue' branch of a conditional expression.");
            Expr right = assign_or_condition();
            expr = new Expr.Conditional(expr, left, right);
        }
        return expr;
    }

    private Expr logic_or() {
        Expr expr = logic_and();
        while(match(OR)) {
            Token operator = previous();
            Expr right = logic_and();
            expr = new Expr.Logical(expr, operator, right);
        }
        return expr;
    }

    private Expr logic_and() {
        Expr expr = logic_xor();
        while(match(AND)) {
            Token operator = previous();
            Expr right = logic_xor();
            expr = new Expr.Logical(expr, operator, right);
        }
        return expr;
    }
    
    private Expr logic_xor() {
        Expr expr = equality();
        while(match(XOR)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, operator, right);
        }
        return expr;
    }

    // an equality expression:
    // expression -> comparison (("==" | "!=") comparison )*
    private Expr equality() {
        Expr expr = comparison();

        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // a comparison expression:
    // comarison -> term ((">" | ">=" | "<" | "<=") term)*
    private Expr comparison() {
        Expr expr = term();

        while(match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // a term expression:
    //term -> factor (("+" | "-") factor)*
    private Expr term() {
        Expr expr = factor();
        
        while(match(PLUS, MINUS)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right); // nesting is here
        }

        return expr;
    }

    //a factor expr:
    // factor -> unary (("*" | "/") unary)*
    private Expr factor() {
        Expr expr = unary();

        while(match(STAR, SLASH)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    // a unary expr:
    // unary -> ("!" | "-") unary | primary;
    private Expr unary() {
        Expr right;
        if(match(BANG, MINUS)) {
            Token operator = previous();
            right = unary(); // recursion
            return new Expr.Unary(operator, right);
        }

        // increment and decrement operators, pre- versions
        if(match(PLUSPLUS, MINUSMINUS)) {
            Token operator = previous();
            Token new_operator;
            if(operator.type == PLUSPLUS)
                new_operator = new Token(PLUS, operator.lexeme, operator.literal, operator.line);
            else
                new_operator = new Token(MINUS, operator.lexeme, operator.literal, operator.line);

            if(!match(IDENTIFIER))
                throw error(peek(), "Expect assignable value for increment/decrement.");

            right = new Expr.Variable(previous());
            return new Expr.Assign(((Expr.Variable) right).name, 
                new Expr.Binary(right, new_operator, new Expr.Literal(1.0)));
        }
        return primary();
    }

    // a primary expr:
    // primary -> NUMBER | STRING | "true" | "false" | "nil" | "("expr")"
    // now, also IDENTIFIER
    private Expr primary() {
        if(match(NUMBER, STRING)) return new Expr.Literal(previous().literal);
        if(match(FALSE)) return new Expr.Literal(false);
        if(match(TRUE)) return new Expr.Literal(true);
        if(match(NIL)) return new Expr.Literal(null);
        if(match(LEFT_PAREN)) {
            // this is a hiddenly large statement.
            Expr expr = expression(); // will have already eaten all the other tokens
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }
        if(match(IDENTIFIER)) return new Expr.Variable(previous());

        // if there is no match for a primary, i.e.
        // an expression must occur here
        throw error(peek(), "Expect expression.");
    }


    // returns true if the current token matches any of what is desired
    // and advances if so
    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    // eats expected token and returns error if not
    private Token consume(TokenType type, String message) {
        if(check(type)) return advance();
        throw error(peek(), message);
    }

    // returns true if the current token is of a given type
    // essentially match without the advance()
    private boolean check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    // advances to the next token and returns the one it just ate
    private Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    // peek() returns a Token object
    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    // self explanatory
    private Token peek() {
        return tokens.get(current);
    }

    // will only throw an error if there is a syntax error...I think?
    private Token previous() {
        return tokens.get(current - 1);
    }

    // **returns**, doesn't throw
    // lets whatever calls this do the throwing
    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    private void synchronize() {
        advance();

        while(!isAtEnd()) {
            // advance tokens until a statement boundary
            if(previous().type == SEMICOLON) return;
            //recall java switches fall through
            switch(peek().type) {
                case CLASS:
                case FUN:
                case VAR:
                case FOR:
                case IF:
                case WHILE:
                case PRINT:
                case RETURN:
                    return;
                default:
            }

            advance();
        }
    }
}
