package lox;

import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable {
	final String name;
	final LoxClass superclass;
	private final Map<String, LoxFunction> methods;

	LoxClass(String name, LoxClass superclass, Map<String, LoxFunction> methods, LoxClass metaclass) {
		super(metaclass);
		this.superclass = superclass;
		this.name = name;
		this.methods = methods;
	}

	public LoxFunction findMethod(String name) {
		if(methods.containsKey(name))
			return methods.get(name);
		if(superclass != null)
			return superclass.findMethod(name);
		return null;
	}

	@Override
	public int arity() {
		LoxFunction initializer = findMethod("init");
		if(initializer == null) return 0;
		return initializer.arity();
	}

	@Override
	public Object call(Interpreter interpreter, List<Object> arguments) {
		LoxInstance instance = new LoxInstance(this);

		// the "init" function just declares and initializes fields
		// just call it as a function (but secretly)
		LoxFunction initializer = findMethod("init");
		if(initializer != null)
			initializer.bind(instance).call(interpreter, arguments);
		return instance;
	}

	@Override
	public String toString() {
		return name;
	}
}
