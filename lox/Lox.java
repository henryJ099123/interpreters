package lox;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class Lox {

    // for error handling
    static boolean hadError = false;
    static boolean hadRuntimeError = false;

    // reuse the same interpreter for each REPL session
    // i.e. global variables should persist
    private static final Interpreter interpreter = new Interpreter();

    // read file from command line or open up interpreter
    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
        System.out.println("Usage: jlox [script]");
        System.exit(64); 
        } else if (args.length == 1) {
        runFile(args[0]);
        } else {
        runPrompt();
        }
    }

    // run from file
    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));

        // indicate an error in the exit code
        if(hadError) System.exit(65);
        if(hadRuntimeError) System.exit(70);
    }

    // run from prompt
    private static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);

        for(;;) {
            System.out.print("> ");
            String line =  reader.readLine();
            if (line == null) break;
            run(line);
            hadError = false;
        }
    }

    // "run" each line (now just print)
    private static void run(String source) {
        Scanner scanner = new Scanner(source);
        List<Token> tokens = scanner.scanTokens(); //recall this is a LIST

        // parse the expression
        Parser parser = new Parser(tokens);
        Expr expression = parser.parse();
        if(hadError) return;
        System.out.println(new AstPrinter().print(expression));
        interpreter.interpret(expression);
    }

    // error handling. it's generally good to separate code that
    // generates the errors and the code that reports said errors
    static void error(int line, String message) {
        report(line, "", message);
    }

    // error handling report format
    private static void report(int line, String where, String message) {
        String err_msg = "[line %d] Error%s: %s";
        System.err.println(String.format(err_msg, line, "", message));
        hadError = true;
    }

    // error handling for an entire token
    static void error(Token token, String message) {
        if (token.type == TokenType.EOF)
            report(token.line, " at end", message);
        else
            report(token.line, "at '" + token.lexeme + "'", message);
    }

    // error handling for a runtime error
    // would be best to show the entire call stack
    static void runtimeError(RuntimeError error) {
        System.err.println(error.getMessage() + "\n[line " + error.token.line + "]");
        hadRuntimeError = true;
    }
}