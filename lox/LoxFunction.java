package lox;

import java.util.List;

public class LoxFunction implements LoxCallable {
    private final String name;
    private final Expr.Fun declaration;
    private final Environment closure;

    LoxFunction(String name, Expr.Fun declaration, Environment closure) {
        this.name = name;
        this.declaration = declaration;
        this.closure = closure;
    }

	LoxFunction bind(LoxInstance instance) {
		Environment environment = new Environment(closure);
		environment.define("this", instance);
		return new LoxFunction(name, declaration, environment);
	} 

    @Override
    public int arity() {
        return declaration.params.size();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment environment = new Environment(closure);
        for(int i = 0; i < declaration.params.size(); i++)
            environment.define(declaration.params.get(i).lexeme, arguments.get(i));
        
        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch(Return r) {
            return r.value;
        }
        return null;
    }

    @Override
    public String toString() {
        if(name == null) return "<fn>";
        return "<fn " + name + ">";
    }
}
