package lox;
import java.util.List;
import java.util.Map;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Scanner;
import java.io.FileWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

//actually going to implement *visitor* actions now
//recall "Object" is root class of Lox types (and is thus return type <?>)
public class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
    
    private static class BreakException extends RuntimeException {}
	private static class ContinueException extends RuntimeException {}
    private static class Exit extends RuntimeException {}
	private static class IOError extends RuntimeException {
		IOError(String msg) {super(msg);}
	}

	private static class LoxScanner {
		private String name;
		boolean isClosed = false;
		private Scanner sc;

		LoxScanner(File f) throws FileNotFoundException {
			this.sc = new Scanner(f);
			this.name = f.getName();
		} 

		String getName() {return name;}

		String nextLine() {
			return sc.nextLine();
		} 

		boolean hasNext() {
			return sc.hasNext();
		} 

		void close() {
			sc.close();
			this.isClosed = true;
		} 

		@Override
		public String toString() {
			return "<file " + this.name + " read>";
		} 
	} 

	private static class LoxWriter extends FileWriter {
		private String name;
		boolean isClosed = false;

		LoxWriter(File f) throws IOException {
			super(f);
			this.name = f.getName();
		} 

		LoxWriter(File f, boolean a) throws IOException {
			super(f, a);
			this.name = f.getName();
		} 

		String getName() {return name;}

		@Override
		public void close() throws IOException {
			super.close();
			this.isClosed = true;
		} 
		
		@Override
		public String toString() {
			return "<file " + this.name + " write>";
		} 
	} 

    // instance of the variable whatnot
    public Environment globals = new Environment();
    private Environment environment = globals;
	private final Map<Expr, Integer> locals = new HashMap<>();

    // native functions
    Interpreter() {
        globals.define("clock", new LoxCallable() {
            @Override
            public int arity() {return 0;}
           
            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                return (double) System.currentTimeMillis() / 1000.0;
            }

            @Override
            public String toString() {
                return "<native fn>";
            }
        });
        globals.define("exit", new LoxCallable() {
            @Override 
            public int arity() {return 0;}

            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                throw new Exit();
            }
        });
		globals.define("length", new LoxCallable() {
			@Override
			public int arity() {return 1;}

			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				if(arguments.get(0) instanceof LoxSequence)
					return (double) ((LoxSequence) arguments.get(0)).length();
				return null;
			} 
		}); 
		globals.define("inputLine", new LoxCallable() {
			@Override
			public int arity() {return 0;}

			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				String userinput = new Scanner(System.in).nextLine();
				return userinput;
			} 
		});
		globals.define("openForRead", new LoxCallable() {
			@Override
			public int arity() {return 1;}

			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				if(!(arguments.get(0) instanceof String))
					throw new IOError("File path must be a string and '" + stringify(arguments.get(0)) + "' is not.");
				File f = new File((String) arguments.get(0));
				if(!f.exists())
					throw new IOError("File '" + stringify(arguments.get(0)) + "' does not exist.");
				if(f.isDirectory())
					throw new IOError("File '" + stringify(arguments.get(0)) + "' is a directory and cannot be read.");
				try {
					return new LoxScanner(f);
				} catch(FileNotFoundException fnfe) {
					throw new IOError("File '" + stringify(arguments.get(0)) + "' cannot be found.");
				} 
			} 
		});
		// will return next line until EOF
		// at that point will then return null
		globals.define("readLine", new LoxCallable() {
			@Override
			public int arity() {return 1;}
			
			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				if(!(arguments.get(0) instanceof LoxScanner))
					throw new IOError("Must pass a file opened for reading to 'readLine'.");
				LoxScanner sc = (LoxScanner) arguments.get(0);
				if(sc.isClosed)
					throw new IOError("Cannot read from a closed file.");
				if(sc.hasNext())
					return sc.nextLine();
				return null;
			} 
		}); 
		globals.define("openForWrite", new LoxCallable() {
			@Override
			public int arity() {return 1;} 

			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				// check if valid file 
				if(!(arguments.get(0) instanceof String))
					throw new IOError("File path must be a string and '" + stringify(arguments.get(0)) + "' is not.");
				File f = new File((String) arguments.get(0));
				/*
				if(!f.exists())
					throw new IOError("File '" + stringify(arguments.get(0)) + "' does not exist or cannot be found.");
				*/
				if(f.isDirectory())
					throw new IOError("File '" + stringify(arguments.get(0)) + "' is a directory and cannot be written.");
				try {
					return new LoxWriter(f);
				} catch(IOException fnfe) {
					throw new IOError("File '" + stringify(arguments.get(0)) + "' cannot be written.");
				} 
			} 
		});
		globals.define("openForAppend", new LoxCallable() {
			@Override
			public int arity() {return 1;} 

			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				// check if valid file 
				if(!(arguments.get(0) instanceof String))
					throw new IOError("File path must be a string and '" + stringify(arguments.get(0)) + "' is not.");
				File f = new File((String) arguments.get(0));
				/*
				if(!f.exists())
					throw new IOError("File '" + stringify(arguments.get(0)) + "' does not exist or cannot be found.");
				*/
				if(f.isDirectory())
					throw new IOError("File '" + stringify(arguments.get(0)) + "' is a directory and cannot be written.");
				try {
					return new LoxWriter(f, true);
				} catch(IOException fnfe) {
					throw new IOError("File '" + stringify(arguments.get(0)) + "' cannot be written.");
				} 
			} 

		});
		globals.define("write", new LoxCallable() {
			@Override
			public int arity() {return 2;}
			
			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				if(!(arguments.get(0) instanceof LoxWriter))
					throw new IOError("Must pass a file opened for writing to 'write'.");
				LoxWriter f = (LoxWriter) arguments.get(0);
				if(f.isClosed)
					throw new IOError("Cannot write to a closed file.");
				try {
					f.write(stringify(arguments.get(1)));
				} catch(IOException e) {
					throw new IOError("Cannot write to file '" + f.getName() + "'.");
				} 
				return null;
			} 
		});
		globals.define("close", new LoxCallable() {
			@Override
			public int arity() {return 1;}

			@Override
			public Object call(Interpreter interpreter, List<Object> arguments) {
				if(arguments.get(0) instanceof LoxWriter) {
					try {
						((LoxWriter) arguments.get(0)).close();
						return null;
					} catch(IOException e) {
						throw new IOError("Cannot close file '" + ((LoxWriter) arguments.get(0)).getName() + "'.");
					} 
				} 
				else if(arguments.get(0) instanceof LoxScanner) {
					((LoxScanner) arguments.get(0)).close();
					return null;
				}
				throw new IOError("Must pass an opened file to 'close'.");
			} 
		});
    }

    // way to interface the entire interpreter
    // boolean is for if it should exit
    boolean interpret(List<Stmt> statements) {
        try {
            for(Stmt statement: statements)
                execute(statement);
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        } catch(Exit exit) {
            return false;
        }
        return true;
    }

	// interfacing with the resolver
	void resolve(Expr expr, int depth) {
		locals.put(expr, depth);
	}

    // just call the "accept" method on this expression
    // which in turn calls the visit method of this statement
    private void execute(Stmt stmt) {
        stmt.accept(this);
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        Object value = null;
        if(stmt.initializer != null)
            value = evaluate(stmt.initializer);
        environment.define(stmt.name.lexeme, value);
        return null;
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function stmt) {
        // environment when the function is *declared*, not *called*
        LoxFunction function = new LoxFunction(stmt.name.lexeme, stmt.function, environment, false);
        environment.define(stmt.name.lexeme, function);
        return null;
    }

	// turn syntax node into LoxClass, a runtime representation
	@Override
	public Void visitClassStmt(Stmt.Class stmt) {
		Object superclass = null;
		if(stmt.superclass != null) {
			superclass = evaluate(stmt.superclass);
			if(!(superclass instanceof LoxClass))
				throw new RuntimeError(stmt.superclass.name, "Superclass must be a class");
		} 
		environment.define(stmt.name.lexeme, null);

		if(stmt.superclass != null) {
			environment = new Environment(environment);
			environment.define("super", superclass);
		} 

		Map<String, LoxFunction> staticMethods = new HashMap<>();
		for(Stmt.Function method: stmt.staticMethods) {
			LoxFunction function = new LoxFunction(method.name.lexeme, method.function, environment, false);
			staticMethods.put(method.name.lexeme, function);
		} 

		LoxClass metaclass = new LoxClass(stmt.name.lexeme + " metaclass", (LoxClass)superclass, staticMethods, null);

		Map<String, LoxFunction> methods = new HashMap<>();
		for(Stmt.Function method: stmt.methods) {
			LoxFunction function = new LoxFunction(method.name.lexeme, method.function, environment, method.name.lexeme.equals("init"));
			methods.put(method.name.lexeme, function);
		}

		LoxClass c = new LoxClass(stmt.name.lexeme, (LoxClass)superclass,  methods, metaclass);

		if(stmt.superclass != null)
			environment = environment.enclosing;

		environment.assign(stmt.name, c);
		return null;
	}

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        Object condition = evaluate(stmt.condition);
        if (isTruthy(condition))
            execute(stmt.ifTrue);
        else if(stmt.ifFalse != null)
            execute(stmt.ifFalse);
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        try {
            while(isTruthy(evaluate(stmt.condition))) {
                try {
                    execute(stmt.body);
                    if(stmt.always_execute != null) execute(stmt.always_execute);
                } catch(ContinueException e) {
                    if(stmt.always_execute != null) execute(stmt.always_execute);
                }
            }
        } catch(BreakException e) {}

        return null;
    }

	@Override
	public Void visitForallStmt(Stmt.Forall stmt) {
		Object sequence = evaluate(stmt.sequence);
		if(!(sequence instanceof LoxSequence))
			throw new RuntimeError(stmt.indName, "Can't iterate over a non-sequence.");
		try {
			int i = 0;
			while(i < ((LoxSequence) sequence).length()) {
				Object value = ((LoxSequence) sequence).getItem(i);
					// must be here
					environment.assignAt(0, stmt.indName, value);
				try {
					execute(stmt.body);
					if(stmt.aftereach != null) execute(stmt.aftereach);
				} catch(ContinueException e) {
					if(stmt.aftereach != null) execute(stmt.aftereach);
				} finally {
					i++;
				} 
			} 
		} catch(BreakException e) {} 

		return null;
	} 

    @Override
    public Void visitBreakStmt(Stmt.Break stmt) {
        throw new BreakException();
    }

    @Override
    public Void visitContinueStmt(Stmt.Continue stmt) {
        throw new ContinueException();
    }

    @Override
    public Void visitBlockStmt(Stmt.Block stmt) {
        // need to define a new environment to handle the new scope
        executeBlock(stmt.statements, new Environment(environment));
        return null;
    }

    void executeBlock(List<Stmt> statements, Environment environment) {
        Environment previous = this.environment;
        try {
            // temporarily change the interpreter's environment
            this.environment = environment;
            for(Stmt statement : statements)
                execute(statement);
        } finally {
            // change back interpreter's environment
            this.environment = previous;
        }
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        System.out.println(stringify(evaluate(stmt.expression)));
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        evaluate(stmt.expression);
        return null;
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = (stmt.value == null) ? null : evaluate(stmt.value);
        throw new Return(stmt.keyword, value);
    }

    // just call the "accept" method on this expression
    // which in turn indirectly calls the visit method of this
    // object on what's correctly inside of it
    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    @Override 
    public Object visitCallExpr(Expr.Call expr) {
        Object callee = evaluate(expr.callee);
        List<Object> arguments = new ArrayList<>();
        for(Expr arg : expr.arguments)
            arguments.add(evaluate(arg));

        if(!(callee instanceof LoxCallable))
            throw new RuntimeError(expr.paren, "Calling an uncallable thing.");
        LoxCallable function = (LoxCallable) callee;
        if(arguments.size() != function.arity()) {
            //System.out.println(arguments.size());
            //System.out.println(function.arity());
            throw new RuntimeError(expr.paren, "Expected " + function.arity() + 
                " arguments but got " + arguments.size() + " instead.");
        }

		try {
			return function.call(this, arguments);
		} catch(IOError r) {
			throw new RuntimeError(expr.paren, r.getMessage());
		} 
    }

	@Override
	public Object visitIndexExpr(Expr.Index expr) {
		Object indexer = evaluate(expr.indexer);
		Object index = evaluate(expr.index);
		int intIndex = checkIndexing(indexer, index, expr.bracket);
		return ((LoxSequence) indexer).getItem(intIndex);
	} 

	@Override
	public Object visitSetIndexExpr(Expr.SetIndex expr) {
		Object indexer = evaluate(expr.indexer);
		Object index = evaluate(expr.index);
		Object value = evaluate(expr.value);
		int intIndex = checkIndexing(indexer, index, expr.bracket);
		if(indexer instanceof LoxString)
			throw new RuntimeError(expr.bracket, "Cannot set index of a string, strings are immutable.");
		((LoxSequence) indexer).setItem(intIndex, value);
		return value;
	} 

	private int checkIndexing(Object indexer, Object index, Token bracket) {
		if(!(indexer instanceof LoxSequence))
			throw new RuntimeError(bracket, "Indexing something that is not a sequence.");
		if(!(index instanceof Double))
			throw new RuntimeError(bracket, "Index to sequence is not a number.");
		int intIndex = ((Double) index).intValue();
		if(intIndex >= ((LoxSequence) indexer).length())
			throw new RuntimeError(bracket, "Indexing out of range of sequence.");
		return intIndex;
	} 

	@Override
	public Object visitGetExpr(Expr.Get expr) {
		Object object = evaluate(expr.object);
		if (object instanceof LoxInstance)
			return ((LoxInstance) object).get(expr.name);
		throw new RuntimeError(expr.name, "Tried to access property from something not an instance of a class.");
	}

	@Override
	public Object visitSetExpr(Expr.Set expr) {
		Object object = evaluate(expr.object);

		if(!(object instanceof LoxInstance))
			throw new RuntimeError(expr.name, "Trying to set field of something not an instance of a class.");
		Object value = evaluate(expr.value);
		((LoxInstance)object).set(expr.name, value);
		return value;
	}

	@Override
	public Object visitSuperExpr(Expr.Super expr) {
		int distance = locals.get(expr);
		LoxClass superclass = (LoxClass)environment.getAt(distance, "super");

		// this is ugly...but it works (this is the instance)
		LoxInstance object = (LoxInstance)environment.getAt(distance - 1, "this");

		LoxFunction method = superclass.findMethod(expr.method.lexeme);
		if(method == null)
			throw new RuntimeError(expr.method, "Undefined property '" + expr.method.lexeme + "' on superclass '" + superclass.name + "'.");
		return method.bind(object);
	} 

	@Override
	public Object visitThisExpr(Expr.This expr) {
		return lookupVariable(expr.keyword, expr);
	} 

    @Override
    public Object visitPostExpr(Expr.Post expr) {
        Object value = evaluate(expr.toReturn);
		evaluate(expr.toAssign);
        //if(!(variable instanceof Double))
        //    throw new RuntimeError(expr.operator, "Cannot increment a non-number.");
		/*
		if(expr instanceof Expr.Variable) {
			if(expr.operator.type == TokenType.PLUSPLUS) {
				evaluate(new Expr.Assign((Expr.Variable) expr.variable).name, new Expr.Binary(expr.variable, new Token(TokenType.PLUS, 
				environment.assign(((Expr.Variable) expr.variable).name, (double) variable + 1.0);
			} else if (expr.operator.type == TokenType.MINUSMINUS)
				environment.assign(((Expr.Variable) expr.variable).name, (double) variable - 1.0);
		} else if(expr instanceof Expr.Get) {
			if(expr.operator.type == TokenType.PLUSPLUS) {
				evaluate(new Expr.Set(
			} 
		} 
		*/
		return value;
    }

    @Override
    public Object visitAssignExpr(Expr.Assign expr) {
        Object value = evaluate(expr.value);
        // environment.assign(expr.name, value);
		Integer distance = locals.get(expr);
		if(distance != null)
			environment.assignAt(distance, expr.name, value);
		else
			globals.assign(expr.name, value);
        return value;
    }

    // evaluating a Conditional expression
    @Override
    public Object visitConditionalExpr(Expr.Conditional expr) {
        if(isTruthy(evaluate(expr.condition)))
            return evaluate(expr.ifTrue);
        return evaluate(expr.ifFalse);
    }

    @Override
    public Object visitLogicalExpr(Expr.Logical expr) {
        Object left = evaluate(expr.left);
        switch(expr.operator.type) {   
            case AND:
                if(!isTruthy(left))
                    return left;
                return evaluate(expr.right);
            case OR:
                if(isTruthy(left))
                    return left;
                return evaluate(expr.right);
            case XOR:
                return isTruthy(left) ^ isTruthy(evaluate(expr.right));
            default:
        }
        // never reached
        return null;
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary expr) {
        Object left = evaluate(expr.left);
        // System.out.println(left);
        Object right = evaluate(expr.right);
        // System.out.println(right);
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
                else if ((left instanceof LoxString && right instanceof Double) || (right instanceof LoxString && left instanceof Double))
                    return loxStringify(stringify(left) + stringify(right));
				else if(left instanceof LoxList && right instanceof LoxList) {
					List<Object> newlist = new ArrayList<>();
					newlist.addAll(((LoxList) left).list);
					newlist.addAll(((LoxList) right).list);
					return new LoxList(newlist);
				} 
				else if(left instanceof LoxString && right instanceof LoxString)
					return loxStringify(stringify(left) + stringify(right));
				System.out.println(left.getClass());
				System.out.println(right.getClass());
                throw new RuntimeError(expr.operator, "Invalid operands.");
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

	@Override
	public Object visitLystExpr(Expr.Lyst expr) {
		List<Object> items = new ArrayList<>();
		for(Expr item: expr.items)
			items.add(evaluate(item));
		return new LoxList(items);
	}

    // implement what happens when we visit a literal expression
    // but that means just getting its value
    @Override
    public Object visitLiteralExpr(Expr.Literal expr) {
		if(expr.value instanceof String)
			return new LoxString((String) expr.value);
        return expr.value;
    }
    
    // just do a general evaluate on the expression of the grouping
    // i.e. do what's inside the parentheses
    @Override
    public Object visitGroupingExpr(Expr.Grouping expr) {
        return evaluate(expr.expression);
    }

    @Override 
    public Object visitVariableExpr(Expr.Variable expr) {
        // return environment.get(expr.name);
		return lookupVariable(expr.name, expr);
    }

	private Object lookupVariable(Token name, Expr expr) {
		Integer distance = locals.get(expr);
		if(distance != null)
			return environment.getAt(distance, name.lexeme);
		return globals.get(name);
	}

    @Override
    public Object visitFunExpr(Expr.Fun expr) {
        return new LoxFunction(null, new Expr.Fun(expr.params, expr.body), environment, false);
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

    // Java's notions of equality are nearly identical to Lox's
    private boolean isEqual(Object a, Object b) {
        if(a == null && b == null) return true;
        if(a == null) return false;
        return a.equals(b);
    }

    static String stringify(Object object) {
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

	static LoxString loxStringify(Object object) {
		return new LoxString(stringify(object));
	} 
}
