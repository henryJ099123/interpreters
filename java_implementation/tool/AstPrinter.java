package lox;

class AstPrinter implements Expr.Visitor<String> {
    String print(Expr expr) {
        return expr.accept(this);
    }

	@Override
	public String visitThisExpr(Expr.This expr) {
		return "";
	}

	@Override
	public String visitSetExpr(Expr.Set expr) {
		return "";
	}

	@Override
	public String visitGetExpr(Expr.Get expr) {
		return "";
	}

    @Override
    public String visitCallExpr(Expr.Call expr) {
        return parenthesize("call", expr.callee);
    }

    @Override
    public String visitPostExpr(Expr.Post expr) {
        return parenthesize("++", expr);
    }

    @Override
    public String visitFunExpr(Expr.Fun expr) {
        return parenthesize("fun", expr);
    }

    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitLogicalExpr(Expr.Logical expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override 
    public String visitAssignExpr(Expr.Assign expr) {
        return parenthesize("=" + expr.name.lexeme, expr.value);
    }

    @Override
    public String visitConditionalExpr(Expr.Conditional expr) {
        return parenthesize("condition", expr.condition, expr.ifTrue, expr.ifFalse);
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) return "nil";
        return expr.value.toString();
    }

    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    @Override
    public String visitVariableExpr(Expr.Variable expr) {
        return parenthesize("var", expr);
    }

    private String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();

        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this));
        }
        builder.append(")");
        return builder.toString();
    }

    // public static void main(String[] args) {
    //     Expr expression = new Expr.Binary(
    //         new Expr.Unary(
    //             new Token(TokenType.MINUS, "-", null, 1),
    //             new Expr.Literal(123)),
    //         new Token(TokenType.STAR, "*", null, 1),
    //         new Expr.Grouping(
    //             new Expr.Literal(45.67)));
    
    //     System.out.println(new AstPrinter().print(expression));
    //   }
    
}
