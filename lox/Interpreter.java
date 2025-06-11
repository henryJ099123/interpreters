package lox;

//actually going to implement *visitor* actions now
//recall "Object" is root class of Lox types (and is thus return type <?>)
public class Interpreter implements Expr.Visitor<Object> {
    
    // implement what happens when we visit a literal expression
    // but that means just getting its value
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
        return expr.value;
    }
    
    // just do a general evaluate on the expression of the grouping
    // i.e. do what's inside the parentheses
    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr.expression);
    }

    // just call the "accept" method on this expression
    // which in turn recursively calls the visit method of this
    // object on what's correctly inside of it
    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    // get the expression and then negate it
    @Override
    public Object visitUnaryExpr(Expr.Unary expr) {
        Object right = evaluate(expr.right);
        switch(expr.operator.type) {
            case MINUS:
                // this type cast is not statically known
                // i.e. this is dynamic typing
                // what if the cast fails?
                checkNumberOperand(expr.operator, right);
                return -(double)right;
            case BANG:
                return !isTruthy(right);
            default:
        }
        // should never be reached
        return null;
    }

    // private method to determine whether something is a number
    // needed for dynamic type checking
    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) return;
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    // same as above but check 2 things
    private void checkNumberOperands(Token operator, Object left, Object right) {
        if(left instanceof Double && right instanceof Double) return;
        throw new RuntimeError(operator, "Operands must be numbers.");
    }
    
    // whether to determine something is true or false
    private boolean isTruthy(Object something) {
        // nil (null) is false
        if(something == null) return false;
        // if it's a Boolean value, it is what it is
        if (something instanceof Boolean) return (boolean)something;
        // otherwise, call it true
        return true;
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluate(expr.left);
        Object right = evaluate(expr.right);
        switch(expr.operator.type) {
            case COMMA:
                return right;
            case MINUS:
                checkNumberOperands(expr.operator, left, right);
                return (double) left - (double) right;
            case SLASH:
                checkNumberOperands(expr.operator, left, right);
                if((double) right == 0)
                    throw new RuntimeError(expr.operator, "Right operand cannot be 0.");
                return (double) left / (double) right;
            // recall the PLUS sign is overloaded.
            // thus we must *dynamically type check*
            case PLUS:
                if(left instanceof Double && right instanceof Double) 
                    return (double) left + (double) right;
                else if (left instanceof String || right instanceof Double)
                    return stringify(left) + stringify(right);
                throw new RuntimeError(expr.operator, "Operands must be two numbers or two strings.");
            case STAR:
                checkNumberOperands(expr.operator, left, right);
                return (double) left * (double) right;
            case LESS:
                checkNumberOperands(expr.operator, left, right);
                return (double) left < (double) right;
            case LESS_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double) left <= (double) right;
            case GREATER:
                checkNumberOperands(expr.operator, left, right);
                return (double) left > (double) right;
            case GREATER_EQUAL:
                checkNumberOperands(expr.operator, left, right);
                return (double) left >= (double) right;
            case EQUAL_EQUAL:
                return isEqual(left, right);
            case BANG_EQUAL:
                return !isEqual(left, right);
            default:
        }
        
        // should be unreachable
        return null;
    }

    // Java's notions of equality are nearly identical to Lox's
    private boolean isEqual(Object a, Object b) {
        if(a == null && b == null) return true;
        if(a == null) return false;
        return a.equals(b);
    }

    // evaluating a Conditional expression
    public Object visitConditionalExpr(Expr.Conditional expr) {
        if(isTruthy(evaluate(expr.condition)))
            return evaluate(expr.ifTrue);
        return evaluate(expr.ifFalse);
    }

    // way to interface the entire interpreter
    void interpret(Expr expression) {
        try {
            Object value = evaluate(expression);
            System.out.println(stringify(value));
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    private String stringify(Object object) {
        // if nil
        if(object == null) return "nil";

        // if a number
        if(object instanceof Double) {
            String text = object.toString();
            // print ints nicely
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }

        // if a string or a boolean
        return object.toString();
    }
}
