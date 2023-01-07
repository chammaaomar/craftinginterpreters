package com.craftinginterpreters.jlox;

import java.util.List;

public class LoxFunction implements LoxCallable {
    private final Stmt.Function declaration;
    LoxFunction(Stmt.Function declaration) {
        this.declaration = declaration;
    }

    public int arity() {
        return declaration.parameters.size();
    }

    // note with this implementation, we can't yet support closures
    // since the parent environment is based on the global, not the environment
    // where the function was declared
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment environment = new Environment(interpreter.globals);
        for (int i = 0; i < declaration.parameters.size(); i++) {
            // parameter binding
            environment.define(declaration.parameters.get(i).lexeme, arguments.get(i));
        }

        try {
            interpreter.executeBlock(declaration.body, environment);
        } catch (Return returnValue) {
            return returnValue.value;
        }
        return null;
    }

    public String toString() {
        return "<fn " + declaration.name.lexeme + ">";
    }


}
