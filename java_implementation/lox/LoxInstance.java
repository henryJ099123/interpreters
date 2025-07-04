package lox;

import java.util.HashMap;
import java.util.Map;

class LoxInstance {
	private LoxClass klass;
	private final Map<String, Object> fields = new HashMap<>();
	private static class klassException extends RuntimeException {}

	LoxInstance(LoxClass klass) {
		this.klass = klass;
	}

	Object get(Token name) {
		// check first if it's a field
		if(fields.containsKey(name.lexeme))
			return fields.get(name.lexeme);

		if(klass != null) {
			// could be a method that is trying to be accessed
			LoxFunction method = klass.findMethod(name.lexeme);
			if(method != null) return method.bind(this);
			/*
			try {
				return klass.get(name);
			} catch(klassException r) {
				throw new RuntimeError(name, "Undefined property '" + name.lexeme + "' on instance of class '" + klass.name + "'.");
			} 
			*/
		}
		throw new RuntimeError(name, "Undefined property '" + name.lexeme + "' on instance of class '" + klass.name + "'.");
		//throw new klassException();
	}

	void set(Token name, Object value) {
		fields.put(name.lexeme, value);
	}

	@Override
	public String toString() {
		return klass.name + " instance";
	}
}
