package lox;

public class Return extends RuntimeException {
    final Object value;
    final Token returnToken;
    
    Return(Token returnToken, Object value) {
        super(null, null, false, false);
        this.value = value;
        this.returnToken = returnToken;
    }
}
