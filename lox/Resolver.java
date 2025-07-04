package lox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
	private enum FunctionType {
		NONE, FUNCTION, INITIALIZER, METHOD
	}

	// i.e. the thing from above, but for classes instead (for `this`)
	private enum ClassType {
		NONE, CLASS, SUBCLASS
	} 

	private class VariableInfo {
		Token token;
		boolean was_defined;
		boolean was_used;

		VariableInfo(Token token, boolean was_defined, boolean was_used) {
			this.token = token;
			this.was_defined = was_defined;
			this.was_used = was_used;
		}
	}

	private final Interpreter interpreter;
	private FunctionType currentFunction = FunctionType.NONE; // for returning outside a function
	private ClassType currentClass = ClassType.NONE; // for returning outside a Class
	private boolean isStatic = false;
	// lexical scopes act as a stack
	// used only for local (block) scopes
	private final Stack<Map<String, VariableInfo>> scopes = new Stack<>();

	Resolver(Interpreter interpreter) {
		this.interpreter = interpreter;
	}

	@Override
	public Void visitBlockStmt(Stmt.Block stmt) {
		beginScope();
		resolve(stmt.statements);
		endScope();
		return null;
	}

	@Override
	public Void visitExpressionStmt(Stmt.Expression stmt) {
		resolve(stmt.expression);
		return null;
	}

	@Override
	// any path can be reached; must reach both.
	public Void visitIfStmt(Stmt.If stmt) {
		resolve(stmt.condition);
		resolve(stmt.ifTrue);
		if(stmt.ifFalse != null) resolve(stmt.ifFalse);
		return null;
	}

	@Override
	public Void visitPrintStmt(Stmt.Print stmt) {
		resolve(stmt.expression);
		return null;
	}

	@Override
	public Void visitWhileStmt(Stmt.While stmt) {
		resolve(stmt.condition);
		resolve(stmt.body);
		if(stmt.always_execute != null) resolve(stmt.always_execute);
		return null;
	}

	@Override
	public Void visitForallStmt(Stmt.Forall stmt) {
		declare(stmt.indName);
		define(stmt.indName);
		resolve(stmt.body);
		if(stmt.aftereach != null) resolve(stmt.aftereach);
		return null;
	} 

	@Override
	public Void visitBreakStmt(Stmt.Break stmt) {
		return null;
	}

	@Override
	public Void visitContinueStmt(Stmt.Continue stmt) {
		return null;
	}

	@Override
	public Void visitReturnStmt(Stmt.Return stmt) {
		if(currentFunction == FunctionType.NONE)
			Lox.error(stmt.keyword, "Can't return outside of a function.", "Semantic");
		if(stmt.value != null) {
			if(currentFunction == FunctionType.INITIALIZER)
				Lox.error(stmt.keyword, "Can't return a value in 'init' constructor.", "Semantic");
			resolve(stmt.value);
		}
		return null;
	}

	@Override
	public Void visitClassStmt(Stmt.Class stmt) {
		ClassType enclosingClass = currentClass;
		currentClass = ClassType.CLASS;

		declare(stmt.name);
		define(stmt.name);

		if(stmt.superclass != null && stmt.name.lexeme.equals(stmt.superclass.name.lexeme)) {
			Lox.error(stmt.superclass.name, "A class cannot inherit from itself.", "Semantic");
		} 
		
		if(stmt.superclass != null) {
			currentClass = ClassType.SUBCLASS;
			resolve(stmt.superclass);
		} 

		if(stmt.superclass != null) {
			beginScope();
			scopes.peek().put("super", new VariableInfo(stmt.name, true, true));
		} 

		beginScope();
		scopes.peek().put("this", new VariableInfo(stmt.name, true, true));

		for(Stmt.Function method: stmt.methods) {
			FunctionType declaration = FunctionType.METHOD;
			if(method.name.lexeme.equals("init"))
				declaration = FunctionType.INITIALIZER;
			resolveFunction(method.function, declaration);
		}

		endScope();
		if(stmt.superclass != null) endScope();

		beginScope();
		isStatic = true;
		scopes.peek().put("this", new VariableInfo(stmt.name, true, true));

		for(Stmt.Function method: stmt.staticMethods) {
			FunctionType declaration = FunctionType.METHOD;
			if(method.name.lexeme.equals("init"))
				Lox.error(method.name, "Can't declare 'init' in static context in class '" + stmt.name.lexeme + "'.", "Semantic");
			resolveFunction(method.function, declaration);
		} 

		isStatic = false;
		endScope();
		currentClass = enclosingClass;
		return null;
	}

	@Override
	public Void visitFunctionStmt(Stmt.Function stmt) {
		declare(stmt.name);
		define(stmt.name);
		resolve(stmt.function);
		return null;
	}

	// separate declaring and defining in order to
	// allow shadowing and weirdo cases
	@Override
	public Void visitVarStmt(Stmt.Var stmt) {
		declare(stmt.name);
		if (stmt.initializer != null)
			resolve(stmt.initializer);

		define(stmt.name);
		return null;
	}

	@Override
	public Void visitAssignExpr(Expr.Assign expr) {
		resolve(expr.value);
		resolveLocal(expr, expr.name, false);
		return null;
	}

	@Override
	public Void visitConditionalExpr(Expr.Conditional expr) {
		resolve(expr.condition);
		resolve(expr.ifTrue);
		
		// I don't think this conditional is needed
		// here just to make me feel better
		if(expr.ifFalse != null) resolve(expr.ifFalse);
		return null;
	}

	@Override
	public Void visitBinaryExpr(Expr.Binary expr) {
		resolve(expr.left);
		resolve(expr.right);
		return null;
	}

	@Override
	public Void visitLogicalExpr(Expr.Logical expr) {
		resolve(expr.left);
		resolve(expr.right);
		return null;
	}

	@Override
	public Void visitCallExpr(Expr.Call expr) {
		resolve(expr.callee);
		for(Expr argument: expr.arguments)
			resolve(argument);
		return null;
	}
	
	@Override
	public Void visitIndexExpr(Expr.Index expr) {
		resolve(expr.indexer);
		resolve(expr.index);
		return null;
	} 

	@Override
	public Void visitSetIndexExpr(Expr.SetIndex expr) {
		resolve(expr.indexer);
		resolve(expr.index);
		resolve(expr.value);
		return null;
	} 

	@Override
	public Void visitGetExpr(Expr.Get expr) {
		// properties are looked up *dynamically* and thus are not resolved
		resolve(expr.object);
		return null;
	}

	@Override
	public Void visitSetExpr(Expr.Set expr) {
		resolve(expr.value);
		resolve(expr.object);
		return null;
	}

	@Override
	public Void visitSuperExpr(Expr.Super expr) {
		if(isStatic)
			Lox.error(expr.keyword, "Cannot use 'super' inside a static method.", "Semantic");
		if(currentClass == ClassType.NONE) 
			Lox.error(expr.keyword, "Cannot use 'super' outside of a class.", "Semantic");
		else if(currentClass != ClassType.SUBCLASS)
			Lox.error(expr.keyword, "Cannot use 'super' in a class with no superclass.", "Semantic");
		resolveLocal(expr, expr.keyword, false);
		return null;
	} 

	@Override
	public Void visitThisExpr(Expr.This expr) {
		if(currentClass == ClassType.NONE)
			Lox.error(expr.keyword, "Cannot use 'this' outside of a class.", "Semantic");
		resolveLocal(expr, expr.keyword, false);
		return null;
	} 

	@Override
	public Void visitGroupingExpr(Expr.Grouping expr) {
		resolve(expr.expression);
		return null;
	}

	@Override
	public Void visitUnaryExpr(Expr.Unary expr) {
		resolve(expr.right);
		return null;
	}

	@Override
	public Void visitPostExpr(Expr.Post expr) {
		resolve(expr.toReturn);
		resolve(expr.toAssign);
		return null;
	}

	@Override
	public Void visitFunExpr(Expr.Fun expr) {
		resolveFunction(expr, FunctionType.FUNCTION);
		return null;
	}

	@Override
	public Void visitLiteralExpr(Expr.Literal expr) {
		return null;
	}

	@Override
	public Void visitLystExpr(Expr.Lyst expr) {
		for(Expr item: expr.items)
			resolve(item);
		return null;
	} 

	@Override
	public Void visitVariableExpr(Expr.Variable expr) {
		// if variable exists, but value is `false`, it is declared but undefined
		if(!scopes.isEmpty() && scopes.peek().containsKey(expr.name.lexeme) &&
				scopes.peek().get(expr.name.lexeme).was_defined == Boolean.FALSE)
			Lox.error(expr.name, "Can't read local variable in its own initializer", "Semantic");
		
		resolveLocal(expr, expr.name, true);
		return null;
	}

	// go into each statement in the block
	void resolve(List<Stmt> statements) {
		for(Stmt statement : statements)
			resolve(statement);
	}

	private void resolve(Stmt stmt) {
		stmt.accept(this);
	}

	private void resolve(Expr expr) {
		expr.accept(this);
	}

	private void resolveFunction(Expr.Fun function, FunctionType type) {
		FunctionType enclosingFunction = currentFunction;
		currentFunction = type;

		beginScope();
		for(Token param : function.params) {
			declare(param);
			define(param);
		}
		resolve(function.body);
		endScope();
		currentFunction = enclosingFunction; // take us back (to whether or not we're in a func)
	}

	private void beginScope() {
		scopes.push(new HashMap<String, VariableInfo>());
	}

	private void endScope() {
		Map<String, VariableInfo> scope = scopes.pop();
		for(VariableInfo variable: scope.values()) {
			if(!variable.was_used)
				Lox.error(variable.token, "Local variable '" + variable.token.lexeme + "' is unused.", "Semantic");
		}
	}

	// should add to the innermost scope so it can shadow an outer one
	// bind its name with a `false` to indicate not ready yet (haven't resolved what it's initialized with)
	private void declare(Token name) {
		if(scopes.isEmpty()) return;
		Map<String, VariableInfo> scope = scopes.peek();
		if(scope.containsKey(name.lexeme))
			Lox.error(name, "Already a variable with name '" + name.lexeme + "' in this scope.", "Semantic");
		scope.put(name.lexeme, new VariableInfo(name, false, false));
	}

	// resolve its initializer expression in the same scope as the new variable
	// set variable value to `true` to mark it as initialized
	private void define(Token name) {
		if(scopes.isEmpty()) return;
		scopes.peek().get(name.lexeme).was_defined = true;
	}

	// start at innermost scope and work outwards
	// if variable is found, it is resolved (and passing the # steps it took to get there)
	// if we get trhough all the scopes, that means it is global
	private void resolveLocal(Expr expr, Token name, boolean updateUse) {
		for(int i = scopes.size() - 1; i >= 0; i--) {
			if(scopes.get(i).containsKey(name.lexeme)) {
				interpreter.resolve(expr, scopes.size() - 1 - i);
				if(updateUse) scopes.get(i).get(name.lexeme).was_used = true;
				return;
			}
		}
		//Lox.error(name, "Variable '" + name.lexeme + "' is used but never declared.", "Semantic");
	}
}
