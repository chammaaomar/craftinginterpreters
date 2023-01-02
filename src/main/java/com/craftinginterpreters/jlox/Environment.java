package com.craftinginterpreters.jlox;

import java.util.HashMap;
import java.util.Map;

public class Environment {
    private final Map<String, Object> values = new HashMap<>();

    void define(String name, Object value) {
        // since we don't check and error out if the variable already exists
        // this code is valid: var a = "123"; var a = "456"
        // not this isn't allowed in e.g., js
        values.put(name, value);
    }

    Object get(Token name) {
        if (values.containsKey(name.lexeme)) {
            return values.get(name.lexeme);
        }

        // make it a runtime error as opposed to a compile error, so we can refer to a variable
        // e.g., in a body of a function, as long as we don't evaluate it before actually assigning it
        // or declaring it. This makes implementing recursive functions easier
        throw new RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }
}
