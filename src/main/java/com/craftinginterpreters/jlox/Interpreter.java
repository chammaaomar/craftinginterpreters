package com.craftinginterpreters.jlox;

import java.util.ArrayList;
import java.util.List;

public class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {

    private Environment globals = new Environment();
    private Environment env = globals;

    Interpreter() {
        globals.define("clock", new LoxCallable() {
            @Override
            public int arity() {
                return 0;
            }

            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                return (double)System.currentTimeMillis() / 1000.0;
            }

            @Override
            public String toString() {
                return "<native fn>";
            }
        });
    }

    public void interpret(List<Stmt> statements) {
        try {
            for (Stmt stmt : statements) {
                execute(stmt);
            }
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    private String stringify(Object object) {
        if (object == null) return "nil";

        if (object instanceof Double) {
            String text = object.toString();
            if (text.endsWith(".0")) {
                return text.substring(0, text.length() - 2);
            }
            return text;
        }

        return object.toString();
    }

    private void checkNumberOperands(Token operator,Object left,Object right) {
        if (left instanceof Double && right instanceof Double) return;
        throw new RuntimeError(operator, "Operands must be numbers.");
    }

    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) return;
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    private boolean isEqual(Object left, Object right) {
        if (left == null && right == null) return true;
        if (left == null) return false;

        return left.equals(right);
    }

    private boolean isTruthy(Object expr) {
        if (expr == Boolean.FALSE || expr == null) return false;
        return true;
    }

    private void execute(Stmt stmt) {
        stmt.accept(this);
    }

    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public Void visitReturnStmt(Stmt.Return stmt) {
        Object value = stmt.expr != null ? evaluate(stmt.expr) : null;

        // technically we can throw anything! Not just exceptions
        // so we use this to escape any statements being executed, no matter how deeply nested
        // inside the function!
        throw new Return(value);
    }

    @Override
    public Void visitFunctionStmt(Stmt.Function function) {
        LoxFunction functionObject = new LoxFunction(function, env);
        env.define(function.name.lexeme, functionObject);
        return null;
    }

    @Override
    public Void visitIfStmt(Stmt.If stmt) {
        if (isTruthy(evaluate(stmt.condition))) {
            execute(stmt.thenBranch);
        } else if (stmt.elseBranch != null) {
            execute(stmt.elseBranch);
        }

        return null;
    }

    public void executeBlock(List<Stmt> statements, Environment blockEnv) {
        Environment enclosingEnv = this.env;
        try {
            this.env = blockEnv;
            for (Stmt stmt : statements) {
                execute(stmt);
            }
        } finally {
            this.env = enclosingEnv;
        }
    }

    @Override
    public Void visitBlockStmt(Stmt.Block block) {
        executeBlock(block.statements, new Environment(this.env));
        return null;
    }

    @Override
    public Void visitVarStmt(Stmt.Var stmt) {
        Object value = null;
        if (stmt.initializer != null) {
            value = evaluate(stmt.initializer);
        }
        env.define(stmt.name.lexeme, value);
        return null;
    }

    @Override
    public Void visitWhileStmt(Stmt.While stmt) {
        while (isTruthy(evaluate(stmt.condition))) {
            execute(stmt.body);
        }
        return null;
    }

    @Override
    public Void visitPrintStmt(Stmt.Print stmt) {
        Object value = evaluate(stmt.expression);
        System.out.println(stringify(value));
        return null;
    }

    @Override
    public Void visitExpressionStmt(Stmt.Expression stmt) {
        evaluate(stmt.expression);
        return null;
    }

    @Override
    public Object visitAssignExpr(Expr.Assign assign) {
        Object value = evaluate(assign.value);
        env.assign(assign.name, value);
        return value;
    }

    @Override
    public Object visitLogicalExpr(Expr.Logical logical) {
        Object left = evaluate(logical.left);

        if (logical.operator.type == TokenType.OR) {
            if (isTruthy(left)) return left;
        } else {
            if (!isTruthy(left)) return left;
        }

        return evaluate(logical.right);
    }

    @Override
    public Object visitLiteralExpr(Expr.Literal literal) {
        return literal.value;
    }

    @Override
    public Object visitVariableExpr(Expr.Variable variable) {
        return env.get(variable.name);
    }

    @Override
    public Object visitGroupingExpr(Expr.Grouping grouping) {
        return evaluate(grouping.expression);
    }

    @Override
    public Object visitUnaryExpr(Expr.Unary unary) {
        Object right = evaluate(unary.right);

        switch (unary.operator.type) {
            case BANG: return !isTruthy(right);
            case MINUS:
                checkNumberOperand(unary.operator, right);
                return -(double)right;
        }

        return null;
    }

    @Override
    public Object visitCallExpr(Expr.Call call) {
        Object callee = evaluate(call.callee);

        if (!(callee instanceof LoxCallable)) {
            throw new RuntimeError(call.paren, "Can only call functions and classes");
        }
        List<Object> args = new ArrayList<>();
        for (Expr arg : call.arguments) {
            args.add(evaluate(arg));
        }

        LoxCallable func = (LoxCallable) callee;

        if (args.size() != func.arity()) {
            throw new RuntimeError(call.paren, "Expected " + func.arity() + " arguments, got " + args.size() + " arguments.");
        }
        return func.call(this, args);
    }

    @Override
    public Object visitBinaryExpr(Expr.Binary binary) {
        Object left = evaluate(binary.left);
        Object right = evaluate(binary.right);

        switch(binary.operator.type) {
            case MINUS:
                checkNumberOperands(binary.operator, left, right);
                return (double)left - (double)right;
            case SLASH:
                checkNumberOperands(binary.operator, left, right);
                return (double)left / (double)right;
            case STAR:
                checkNumberOperands(binary.operator, left, right);
                return (double)left * (double)right;
            case PLUS:
                if (left instanceof Double && right instanceof Double) {
                    return (double)left + (double)right;
                }
                if (left instanceof String && right instanceof String) {
                    return (String)left + (String)right;
                }
                throw new RuntimeError(binary.operator, "Operands must both be numbers or strings.");

            case GREATER:
                checkNumberOperands(binary.operator, left, right);
                return (double)left > (double)right;
            case GREATER_EQUAL:
                checkNumberOperands(binary.operator, left, right);
                return (double)left >= (double)right;
            case LESS:
                checkNumberOperands(binary.operator, left, right);
                return (double)left < (double)right;
            case LESS_EQUAL:
                checkNumberOperands(binary.operator, left, right);
                return (double)left <= (double)right;
            case EQUAL_EQUAL:
                return isEqual(left, right);
            case BANG_EQUAL:
                return !isEqual(left, right);
        }

        return null;
    }
}
