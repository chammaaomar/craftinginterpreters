package com.craftinginterpreters.jlox;

abstract class Expr {
    // the visitor design pattern allows us to approximate a functional style of programming within an
    // Object-oriented language such as Java. Object-oriented languages make it easy to add new classes
    // since you don't have to open up existing classes. Functional style of programming makes it easy
    // to add new functions, since you don't have to touch existing functions, and just add a function with
    // pattern matching to act on different subclasses. This design pattern is appropriate for the Nodes in our
    // Abstract Syntax Tree (AST), since it gives us separation of concerns, where The Interpreter code can live
    // on its own, and not live with name resolution code or static type checking code, since they all act on the
    // AST Nodes, so makes since to use this "functional" style of programming
    interface Visitor<R> {
        R visitBinaryExpr(Binary binary);
        R visitGroupingExpr(Grouping grouping);
        R visitLiteralExpr(Literal literal);
        R visitUnaryExpr(Unary unary);
        R visitVariableExpr(Variable variable);
    }

    abstract <R> R accept(Visitor<R> visitor);
    static class Binary extends Expr {
        final Expr left;
        final Token operator;
        final Expr right;
        Binary(Expr left, Token operator, Expr right) {
            this.left = left;
            this.operator = operator;
            this.right = right;
        }

        @Override
        <R> R accept(Visitor<R> visitor) {
            return visitor.visitBinaryExpr(this);
        }
    }
    static class Grouping extends Expr {
        final Expr expression;
        Grouping(Expr expression) {
            this.expression = expression;
        }

        @Override
        <R> R accept(Visitor<R> visitor) {
            return visitor.visitGroupingExpr(this);
        }
    }
    static class Literal extends Expr {
        final Object value;
        Literal(Object value) {
            this.value = value;
        }

        @Override
        <R> R accept(Visitor<R> visitor) {
            return visitor.visitLiteralExpr(this);
        }
    }
    static class Unary extends Expr {
        final Token operator;
        final Expr right;
        Unary(Token operator, Expr right) {
            this.operator = operator;
            this.right = right;
        }

        @Override
        <R> R accept(Visitor<R> visitor) {
            return visitor.visitUnaryExpr(this);
        }
    }
    static class Variable extends Expr {
        final Token name;
        Variable(Token name) {
            this.name = name;
        }

        @Override
        <R> R accept(Visitor<R> visitor) {
            return visitor.visitVariableExpr(this);
        }
    }
}
