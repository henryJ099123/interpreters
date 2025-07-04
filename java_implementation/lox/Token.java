package lox;

class Token {
    final TokenType type; //type of lexeme
    final String lexeme; // the actual string contents
    final Object literal; // if it is a literal
    final int line; // line it occurred on

    Token(TokenType type, String lexeme, Object literal, int line) {
        this.type = type;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line;
    }

    public String toString() {
        return type + " " + lexeme + " " + literal;
    }

}
