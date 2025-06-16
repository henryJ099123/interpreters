package tool;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

// helper file to generate the java file with
// the class structure for the parse grammar
public class GenerateAst {
    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.err.println("Usage: generate_ast <output directory>");
            System.exit(64);
        }
        String outputDir = args[0];

        // now define a file for the STATEMENTS
        defineAst(outputDir, "Stmt", Arrays.asList(
            "Block : List<Stmt> statements",
            "Break : ",
            "Continue : ",
            "Function : Token name, Expr.Fun function",
            "Expression : Expr expression",
            "While : Expr condition, Stmt body, Stmt always_execute",
            "If : Expr condition, Stmt ifTrue, Stmt ifFalse",
            "Print : Expr expression",
            "Return : Token keyword, Expr value",
            "Var : Token name, Expr initializer"
        ));

        // EXPRESSIONS
        // for each rule, have the arguments be other nonterminals for that rule
        defineAst(outputDir, "Expr", Arrays.asList(
            "Binary   : Expr left, Token operator, Expr right",
            "Logical : Expr left, Token operator, Expr right",
            "Assign : Token name, Expr value",
            "Fun : List<Token> params, List<Stmt> body",
            "Post : Token operator, Expr.Variable variable", 
            "Call : Expr callee, Token paren, List<Expr> arguments",
            "Conditional    : Expr condition, Expr ifTrue, Expr ifFalse", 
            "Grouping : Expr expression",
            "Literal  : Object value",
            "Unary    : Token operator, Expr right",
            "Variable : Token name"
        ));
    }

    private static void defineAst(
            String outputDir, String basename, List<String> types)
            throws IOException {
        String path = outputDir + "/" + basename + ".java";
        PrintWriter writer = new PrintWriter(path, "UTF-8");

        writer.println("package lox;");
        writer.println("import java.util.List;\n");
        writer.println("abstract class " + basename + " {");

        defineVisitor(writer, basename, types);

        for (String type: types) {
            String className = type.split(":")[0].trim();
            String fields = type.split(":")[1].trim();
            defineType(writer, basename, className, fields);
        }

        writer.println();
        writer.println("    abstract <R> R accept(Visitor<R> visitor);");

        writer.println("}");
        writer.close();

    }

    private static void defineVisitor(
            PrintWriter writer, String basename, List<String> types) {
        writer.println("    interface Visitor<R> {");

        for (String type : types) {
            String typeName = type.split(":")[0].trim();
            writer.println("        R visit" + typeName + basename + "(" +
                typeName + " " + basename.toLowerCase() + ");");
        }

        writer.println("    }");
    }

    private static void defineType(
            PrintWriter writer, String basename, String className, String fieldList) {
        // class declaration
        writer.println("    static class " + className + " extends " + basename + " {");

        //constructor
        writer.println("        " + className + "(" + fieldList + ") {");
        
        String[] fields;
        if(fieldList.isEmpty())
            fields = new String[0];
        else fields = fieldList.split(", ");
        // store the data from parameters
        for (String field : fields ) {
            String name = field.split(" ")[1];
            writer.println("            this." + name + " = " + name + ";");
        }

        writer.println("        }");

        writer.println();
        writer.println("        @Override");
        writer.println("        <R> R accept(Visitor<R> visitor) {");
        writer.println("            return visitor.visit" + className + basename + "(this);");
        writer.println("        }\n");

        // fields
        for (String field : fields)
            writer.println("        final " + field + ";");
        writer.println("    }\n");
    }
}