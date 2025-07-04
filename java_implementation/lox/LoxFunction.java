package lox;

import java.util.List;

public class LoxFunction implements LoxCallable {
    private final String name;
    private final Expr.Fun declaration;
    private final Environment closure;
    private final boolean isInitializer;

    LoxFunction(String name, Expr.Fun declaration, Environment closure, boolean isInitializer) {
		this.isInitializer = isInitializer;
        this.name = name;
        this.declaration = declaration;
        this.closure = closure;
    }

	LoxFunction bind(LoxInstance instance) {
		Environment environment = new Environment(closure);
		environment.define("this", instance);
		return new LoxFunction(name, declaration, environment, isInitializer);
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
			// allow early return
			if(isInitializer) return closure.getAt(0, "this");
            return r.value;
        }

		// forcibly return `this`n 
		if (isInitializer) return closure.getAt(0, "this");
        return null;
    }

    @Override
    public String toString() {
        if(name == null) return "<fn>";
        return "<fn " + name + ">";
    }
}
