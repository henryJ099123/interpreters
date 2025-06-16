package lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static lox.TokenType.*; 

class Scanner {
    private static final Map<String, TokenType> keywords;

    // reserved words hashmap
    static {
        keywords = new HashMap<>();
        keywords.put("and", AND);
        keywords.put("class", CLASS);
        keywords.put("else", ELSE);
        keywords.put("false", FALSE);
        keywords.put("for", FOR);
        keywords.put("fun", FUN);
        keywords.put("if", IF);
        keywords.put("nil", NIL);
        keywords.put("or", OR);
        keywords.put("xor", XOR);
        keywords.put("print", PRINT);
        keywords.put("return", RETURN);
        keywords.put("super", SUPER);
        keywords.put("this", THIS);
        keywords.put("true", TRUE);
        keywords.put("var", VAR);
        keywords.put("while", WHILE);
        keywords.put("break", BREAK);
        keywords.put("continue", CONTINUE);
        keywords.put("aftereach", AFTEREACH);
        
    }

    private final String source;
    private final List<Token> tokens = new ArrayList<>();

    // fields to track where scanner is in the source text file
    private int start = 0; // first char of lexeme being scanned
    private int current = 0; // character currently being considered
    private int line = 1;

    Scanner(String source) {
        this.source = source;
    }

    List<Token> scanTokens() {
        while (!isAtEnd()) {
            // start of next lexeme
            start = current;
            // add the next token to the list of tokens
            scanToken();
        }
        // appends the EOF token
        tokens.add(new Token(EOF, "", null, line));
        return tokens;
    }

    // scan a token from the current spot in the source file
    private void scanToken() {
        //for single character tokens:
        char c = advance();
        switch(c) {
            case '(': addToken(LEFT_PAREN); break;
            case ')': addToken(RIGHT_PAREN); break;
            case '{': addToken(LEFT_BRACE); break;
            case '}': addToken(RIGHT_BRACE); break;
            case ',': addToken(COMMA); break;
            case '.': addToken(DOT); break;
            case '+': 
                if(match('+')) addToken(PLUSPLUS);
                else if(match('=')) addToken(PLUS_EQUAL);
                else addToken(PLUS); 
                break;
            case '-': 
                if(match('-')) addToken(MINUSMINUS);
                else if(match('=')) addToken(MINUS_EQUAL);
                else addToken(MINUS); 
                break;
            case '*': 
                addToken(match('=') ? STAR_EQUAL : STAR); break; 
            case '?': addToken(QUESTION); break;
            case ':': addToken(COLON); break;
            case ';': addToken(SEMICOLON); break;

            // characters that initiate some special lexemes
            case '!': 
                addToken(match('=') ? BANG_EQUAL : BANG);
                break;
            case '=':
                addToken(match('=') ? EQUAL_EQUAL : EQUAL);
                break;
            case '<':
                addToken(match('=') ? LESS_EQUAL : LESS);
                break;
            case '>':
                addToken(match('=') ? GREATER_EQUAL : GREATER);
                break;
            case '/':
                if (match('/')) {
                    //this means this is a comment. **NO** addToken call.
                    while(peek() != '\n' && !isAtEnd()) advance();

                // multiline comments allowed, but not nested
                } else if (match('*')) {
                    while(peek() != '*' && peekNext() != '/' && !isAtEnd()) {
                        if(peek() == '\n') line++;
                        advance();
                    }
                    if(isAtEnd()) Lox.error(line, "Unterminated comment.");
                    else {advance(); advance();} // eat the */
                } else if(match('=')) {
                    addToken(SLASH_EQUAL);
                } else {
                    addToken(SLASH);
                }
                break;
            // ignore whitespace
            case ' ':
            case '\r':
            case '\t':
                break;
            // increment line in case of a newline
            case '\n':
                line++;
                break;

            //now, for literals:
            case '"': string(); break;

            // does not BREAK scanning, and erroneous char still consumed.
            // want to identify as many errors as possible without executing code.
            default:
                if(isDigit(c)) {
                    number();
                // any lexeme beginning with [A-Za-z_] is assumed an identifier, for now
                } else if (isAlpha(c)) {
                    identifier();
                } else {
                    Lox.error(line, "Unexpected character.");
                }
                break;
        }
    }

    // add either a variable name or a keyword
    private void identifier() {
        while(isAlphaNumeric(peek())) advance();

        String text = source.substring(start, current);
        TokenType type = keywords.get(text);
        if (type == null) type = IDENTIFIER;
        addToken(type);
    }

    /* *************** HELPER FUNCTIONS *************** */

    private boolean isAtEnd() {
        return current >= source.length(); // true if we are past EOF
    }

    // consumes the next character in source file and returns it
    private char advance() {
        return source.charAt(current++);
    }

    // grabs the text of current lexeme and creates a new token for it
    private void addToken(TokenType type) {
        addToken(type, null);
    }

    private void addToken(TokenType type, Object literal) {
        String text = source.substring(start, current);
        tokens.add(new Token(type, text, literal, line));
    }

    // recall the peek/parse stacks? this peeks at the next character
    // to see if it is a different lexeme
    private boolean match(char expected) {
        if (isAtEnd()) return false;
        if (source.charAt(current) != expected) return false;
        current++;
        return true;
    }

    // lookahead (peek at the input string)
    // the smaller the lookahead is of a parser, the faster the scanner is
    private char peek() {
        if (isAtEnd()) return '\0';
        return source.charAt(current);
    }

    //allows lookahead of two characters
    private char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source.charAt(current + 1);
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
         c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }
    // function that starts a literal for a string
    private void string() {
        // increment current until end of string, as that's the whole token
        // increase line for multiline strings
        while(peek() != '"' && !isAtEnd()) {
            if(peek() == '\n') line++;
            advance();
        }

        // unterminated string
        if (isAtEnd()) {
            Lox.error(line, "Unterminated string.");
            return;
        }
        // get the closing "
        advance();

        // Trim the surrounding quotes
        String value = source.substring(start + 1, current - 1);
        addToken(STRING, value);
    }

    //function to consume a number literal
    private void number() {
        // keep grabbing numbers
        while(isDigit(peek())) advance();

        // if there's a decimal place recognize it
        // and keep adding numbers
        if(peek() == '.' && isDigit(peekNext())) {
            advance();
            
            while(isDigit(peek())) advance();
        }

        addToken(NUMBER, Double.parseDouble(source.substring(start, current)));
    }
}
