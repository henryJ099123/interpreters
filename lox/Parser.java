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
    private int loop_depth = 0;
    
    // constructor
    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    // the initial statement!
    List<Stmt> parse() {
        // for(Token token: tokens) {
        //     System.out.print(token.type + " ");
        // }
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

    /* *************** DECLARATIONS **************** */

    private Stmt declaration() {
        try {
            if(check(FUN)) {
				if(checkNext(IDENTIFIER)) {
					consume(FUN, "Function delcaration expected.");
                	return function("function");
				}
				return expressionStatement();
			}
            if(match(VAR)) return varDeclaration();
			if(match(CLASS)) return classDeclaration();
            return statement();
        } catch(ParseError error) {
            synchronize();
            return null;
        }
    }

	private Stmt classDeclaration() {
		Token name = consume(IDENTIFIER, "Expect class name.");
		consume(LEFT_BRACE, "Expect '{' before class body.");
		List<Stmt.Function> methods = new ArrayList<>();

		// check for end JIC
		while(!check(RIGHT_BRACE) && !isAtEnd())
			methods.add(function("method"));

		consume(RIGHT_BRACE, "Expect '}' after class body.");
		return new Stmt.Class(name, methods);
	}

    private Stmt.Function function(String kind) {
        Token name = consume(IDENTIFIER, "Expect " + kind + " name.");
        return new Stmt.Function(name, functionExpr());
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

    /* *************** STATEMENTS (general) **************** */

    private Stmt statement() {
        if(match(PRINT)) return printStatement();
        if(match(LEFT_BRACE)) return new Stmt.Block(block());
        if(match(IF)) return ifStatement();
        if(match(WHILE)) return whileStatement();
        if(match(FOR)) return forStatement();
        if(match(BREAK)) return breakStatement();
        if(match(CONTINUE)) return continueStatement();
        if(match(RETURN)) return returnStatement();
        return expressionStatement();
    }

    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    private Stmt returnStatement() {
        Token keyword = previous(); // for errors, to get line
        Expr value = null;
        if(!check(SEMICOLON))
            value = expression();
        consume(SEMICOLON, "Expect ';' after return value.");
        return new Stmt.Return(keyword, value);
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
        loop_depth++;
        Stmt body = statement();
        loop_depth--;
        Stmt aftereach = null;
        if(match(AFTEREACH)) {
            aftereach = statement();
        }
        return new Stmt.While(condition, body, aftereach);
    }

    private Stmt forStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'for'.");

        Stmt initializer;
        if(match(SEMICOLON)) initializer = null;
        else if(match(VAR)) initializer = varDeclaration();
        else initializer = expressionStatement();

        // a "blank" condition means true for a for loop
        Expr condition = new Expr.Literal(true);
        if(!check(SEMICOLON))
            condition = expression();
        consume(SEMICOLON, "Expect ';' after loop condition.");

        // get the increment if its there
        Expr increment = null;
        if(!check(RIGHT_PAREN))
            increment = expression();
        consume(RIGHT_PAREN, "Expect ')' after loop increment.");

        // for break and continue
        loop_depth++;
        Stmt body = statement();
        loop_depth--;

        // some additional functionality I added
        Stmt aftereach = null;
        if(match(AFTEREACH))
            aftereach = statement();
        
        // modifies the aftereach as necessary
        if(increment != null) {
                aftereach = (aftereach == null) ? new Stmt.Expression(increment) :
                    new Stmt.Block(Arrays.asList(aftereach, new Stmt.Expression(increment)));
        }

        body = new Stmt.While(condition, body, aftereach);
        
        // adds initializer at the beginning
        if(initializer != null)
            body = new Stmt.Block(Arrays.asList(initializer, body));

        return body;
    }

    // creates a new break statement
    private Stmt breakStatement() {
        consume(SEMICOLON, "Expect ';' after 'break'.");
        if(loop_depth == 0)
            error(previous(), "'break' must occur within a loop.");
        return new Stmt.Break();
    }

    // creates a new continue statement
    private Stmt continueStatement() {
        consume(SEMICOLON, "Expect ';' after 'continue'.");
        if(loop_depth == 0)
            error(previous(), "'continue' must occur within a loop.");
        return new Stmt.Continue();
    }

    // creates a new expression statement
    private Stmt expressionStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Expression(value);
    }

    /*  the reason this returns a list of statements
     *  rather than a Block statement is because this code
     *  will be reused for function bodies */
    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<>();
        while(!check(RIGHT_BRACE) && !isAtEnd())
            statements.add(declaration());
        consume(RIGHT_BRACE, "Expect '}' after block.");
        return statements;
    }


    /* *************** EXPRESSIONS **************** */

    // nonterminal rules translate to function code
    private Expr expression() {
        return comma();
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
        if(match(EQUAL, PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL)) {
            Token operator = previous();
            Expr value = assign_or_condition();
            
            // if its a variable it'll boil down to an Expr.Variable
            // then we can extract its name token and return the correct assignment expression
            if (expr instanceof Expr.Variable) {
				Token name = ((Expr.Variable) expr).name;

				// switch based off of all the options
				Token new_operator = null; // should never happen.
				switch(operator.type) {
					case EQUAL:
						return new Expr.Assign(name, value);
					case PLUS_EQUAL:
						new_operator = new Token(PLUS, operator.lexeme, operator.literal, operator.line);
						break;
					case MINUS_EQUAL:
						new_operator = new Token(MINUS, operator.lexeme, operator.literal, operator.line);
						break;
					case STAR_EQUAL:
						new_operator = new Token(STAR, operator.lexeme, operator.literal, operator.line);
						break;
					case SLASH_EQUAL:
						new_operator = new Token(SLASH, operator.lexeme, operator.literal, operator.line);
						break;
					default:
            	}

            	return new Expr.Assign(name, new Expr.Binary(expr, new_operator, value));

			} else if(expr instanceof Expr.Get) {
				Expr.Get get = (Expr.Get) expr;
				return new Expr.Set(get.object, get.name, value);
			} else
				error(operator, "Invalid assignment to left-hand side.");

        } else if(match(QUESTION)) {
            Expr left = expression();
            consume(COLON, "Expect ':' after the 'ifTrue' branch of a conditional expression.");
            Expr right = assign_or_condition();
            expr = new Expr.Conditional(expr, left, right);

		}

        return expr;
    }

    // logic expressions (allow short circuiting)
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
    
    // literally nothing new here. This could've been a binary one
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
        return call();
    }

    // when something does () or ++ or -- or . afterward
    private Expr call() {
        Expr expr = primary();
        while(true) {
            if(match(LEFT_PAREN)) {
                expr = finishCall(expr);
            } else if(match(DOT)) {
				Token name = consume(IDENTIFIER, "Expect property name after calling '.'");
				expr = new Expr.Get(expr, name);
			} else if(match(PLUSPLUS, MINUSMINUS)) {
				// there is no concatening () or . with a ++. this is the last one
                return post(expr, previous());
			} else {
                break;
			}
        }
        return expr;
    }

    // for the ++ and --
    private Expr post(Expr expr, Token operator) {
        if(!(expr instanceof Expr.Variable))
            throw error(operator, "Calling " + operator.lexeme + " on an unassignable expression.");
        return new Expr.Post(operator, (Expr.Variable) expr);
    }

    // for actually completing a function call
    private Expr finishCall(Expr callee) {
        List<Expr> arguments = new ArrayList<>();
        if(!check(RIGHT_PAREN)) {
            do {
                if(arguments.size() >= 255)
                    error(peek(), "Maximum number of arguments is 255.");
                arguments.add(assign_or_condition());
            } while(match(COMMA));
        }
        Token paren = consume(RIGHT_PAREN, "Expect '(' after arguments.");
        return new Expr.Call(callee, paren, arguments);
    }

    // a primary expr:
    // primary -> NUMBER | STRING | "true" | "false" | "nil" | "("expr")"
    // now, also IDENTIFIER or an anonymous function
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
		if(match(THIS)) return new Expr.This(previous());
        if(match(IDENTIFIER)) return new Expr.Variable(previous());
        if(match(FUN)) return functionExpr();
        // if there is no match for a primary, i.e.
        // an expression must occur here
        throw error(peek(), "Expect expression.");
    }

    private Expr.Fun functionExpr() {
        consume(LEFT_PAREN, "Expect '(' after declaring function.");
        List<Token> parameters = new ArrayList<>();
        if(!check(RIGHT_PAREN)) {
            do {
                if(parameters.size() >= 255)
                    error(peek(), "Can't have more than 255 parameters.");
                parameters.add(consume(IDENTIFIER, "Expect parameter for function."));
            } while(match(COMMA));
        }
        consume(RIGHT_PAREN, "Expect ')' for function declaration.");
        consume(LEFT_BRACE, "Expect '{' to start of body for function.");
        List<Stmt> body = block();
        return new Expr.Fun(parameters, body);
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

	private boolean checkNext(TokenType type) {
		if(isAtEnd() || tokens.get(current + 1).type == EOF)
			return false;
		return tokens.get(current + 1).type == type;
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
        Lox.error(token, message, "Parse");
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
