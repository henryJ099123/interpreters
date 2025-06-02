package lox;

import java.util.List;

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
    Expr parse() {
        try {
            return expression();
        } catch(ParseError error) { // why do anything else if an error is found!
            return null;
        }
    }

    // nonterminal rules translate to function code
    private Expr expression() {
        return equality();
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
        if(match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary(); // recursion
            return new Expr.Unary(operator, right);
        }
        return primary();
    }

    // a primary expr:
    // primary -> NUMBER | STRING | "true" | "false" | "nil" | "("expr")"

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

        // if there is no match for a primary, i.e.
        // an expression must occur here
        throw error(peek(), "Expect expression.")
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
