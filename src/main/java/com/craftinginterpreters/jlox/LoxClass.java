package com.craftinginterpreters.jlox;

import java.util.List;

public class LoxClass implements LoxCallable {
    final String name;

    LoxClass(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return name;
    }

    public int arity() {
        return 0;
    }

    public Object call(Interpreter interpreter, List<Object> arguments) {
        return null;
    }
}
