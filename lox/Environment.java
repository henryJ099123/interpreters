package lox;

import java.util.HashMap;
import java.util.Map;

public class Environment {
    private final Map<String, Object> values = new HashMap<>();
    private Environment enclosing; // pointer to "parent"

    // for global scope
    Environment() {
        this.enclosing = null;
    }

    // for local scope
    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }
    
    // define a new variable
    void define(String name, Object value) {
        values.put(name, value);
    }

    // access a variable
    Object get(Token name) {
        if(values.containsKey(name.lexeme))
            return values.get(name.lexeme);
        
        // look up a level if it isn't here
        if(enclosing != null) return enclosing.get(name);

        // no variable at all
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

	// get the thing at the proper number of environments
	// it *knows* it must be here because of the resolver
	Object getAt(int distance, String name) {
		return get_ancestor(distance).values.get(name);
	}

	void assignAt(int distance, Token name, Object value) {
		// don't need to go through `assign` because the key is here?
		// we know it's good via the map in Resolver.java
		return get_ancestor(distance).values.put(name.lexeme, value);
	}

	// go up proper number of environments
	Environment get_ancestor(int distance) {
		Environment environment = this;
		for(int i = 0; i < distance; i++)
			environment = environment.enclosing;
		return environment;
	}

    // assignment cannot create a new variable
    void assign(Token name, Object value) {
        if(values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }
        // look up a level if it isn't here
        if(enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        // not here at all
        throw new RuntimeError(name, "Undefined variable '"+name.lexeme+"'.");
    }

}
