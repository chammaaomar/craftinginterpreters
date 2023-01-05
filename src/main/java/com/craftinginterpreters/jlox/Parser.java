package com.craftinginterpreters.jlox;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static com.craftinginterpreters.jlox.TokenType.*;

public class Parser {

    private static class ParseError extends RuntimeException {}
    private final List<Token> tokens;
    private int current = 0;

    List<Stmt> parse() {
        List<Stmt> statements = new ArrayList<>();
        while (!isAtEnd()) {
            statements.add(declaration());
        }

        return statements;
    }

    // the nesting defines the precedence rules for parsing statements
    // the precedence rules are: variable declaration statement < print statement | expression statement,
    // and we're using recursive descent parsing, so in order to evaluate a variable declaration statement
    // we have to evaluate any sub-statements;

    private Stmt declaration() {
        try {
            if (match(VAR)) return varDeclaration();
            return statement();
        } catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    private Stmt varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name.");

        Expr initializer = null;
        if (match(EQUAL)) initializer = expression();

        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }

    private Stmt statement() {
        if (match(PRINT)) return printStatement();
        if (match(LEFT_BRACE)) return blockStatement();
        if (match(IF)) return ifStatement();
        if (match(WHILE)) return whileStatement();
        // we will desugar the for statement, i.e. parse it into while AST node
        if (match(FOR)) return forStatement();

        return expressionStatement();
    }

    private Stmt whileStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'while' statement");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after 'while' statement condition");
        Stmt body = statement();
        return new Stmt.While(condition, body);
    }

    private Stmt forStatement() {
        // example: for (var i = 0; i < 10; i = i + 1) print i;
        consume(LEFT_PAREN, "Expect '(' after 'for' statement");
        Stmt initializer;
        if (match(SEMICOLON)) {
            initializer = null;
        } else if (match(VAR)) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        // just runs without condition by default
        Expr condition = new Expr.Literal(true);
        if (!check(SEMICOLON)) {
            condition = expression();
        }
        consume(SEMICOLON, "Expect ';' after 'for' statement condition");

        Expr increment = null;
        if (!check(RIGHT_PAREN)) {
            increment = expression();
        }

        consume(RIGHT_PAREN, "Expect ')' after 'for' statement");
        Stmt body = statement();

        // the original body of the 'for' loop, followed by the increment
        // example desugraing for the following loop: for (var i = 0; i < 10; i = i + 1) print i;
        /*
        {
          var i = 0;
          while (i < 10) {
            print i;
            i = i + 1;
          }
        }
        */
        body = new Stmt.Block(Arrays.asList(
                body,
                new Stmt.Expression(increment)
        ));

        return new Stmt.Block(Arrays.asList(
              initializer,
                new Stmt.While(condition, body)
        ));
    }

    private Stmt ifStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'if' statement");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after 'if' statement condition");
        Stmt thenBranch = statement();
        if (match(ELSE)) {
            Stmt elseBranch = statement();
            return new Stmt.If(condition, thenBranch, elseBranch);
        } else {
            return new Stmt.If(condition, thenBranch, new Stmt.Block(null));
        }
    }

    private Stmt blockStatement() {
        List<Stmt> statements = new ArrayList<>();
        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            statements.add(declaration());
        }

        consume(RIGHT_BRACE, "Expect '}' after block");
        return new Stmt.Block(statements);
    }

    private Stmt printStatement() {
        Expr expr = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(expr);
    }

    private Stmt expressionStatement() {
        Expr expr = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(expr);
    }

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(current);
    }

    private Token previous() {
        return tokens.get(current - 1);
    }

    private boolean check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    private Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    private boolean match(TokenType... types) {
        for (TokenType type: types) {
            if (check(type)) {
                advance();
                return true;
            }
        }

        return false;
    }

    // the nesting defines the precedence rules in the language
    // our recursive descent parser starts with parsing the lowest precedence expressions
    // and in order to evaluate the lowest precedence expression it has to evaluate any
    // higher-precedence subexpressions. The precedence order in parsing tokens into
    // expressions: equality (!= ==) < comparison (> >= < <=) < term (+-) < factor (*/) < unary (! -) < primary (literals / atoms)

    private Expr expression() {
        return assignment();
    }

    private Expr assignment() {
        Expr expr = or();
        if (match(EQUAL)) {
            Token equals = previous();
            Expr rhs = assignment();
            if (expr instanceof Expr.Variable) {
                return new Expr.Assign(((Expr.Variable) expr).name, rhs);
            }
            error(equals, "Must be a valid assignment target.");
        }
        return expr;
    }

    private Expr or() {
        Expr expr = and();
        while (match(OR)) {
            Token operator = previous();
            Expr right = and();
            expr = new Expr.Logical(expr, operator, right);
        }
        return expr;
    }

    private Expr and() {
        Expr expr = equality();
        while (match(AND)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    private Expr equality() {
        Expr expr = comparison();
        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr comparison() {
        Expr expr = term();

        while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = previous();
            Expr right = term();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr term() {
        Expr expr = factor();

        while (match(MINUS, PLUS)) {
            Token operator = previous();
            Expr right = factor();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr factor() {
        Expr expr = unary();

        while (match(STAR, SLASH)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }

        return expr;
    }

    private Expr unary() {
        if (match(MINUS, BANG)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        } else {
            return call();
        }

    }

    private Expr call() {
        Expr callee = primary();
        if (match(LEFT_PAREN)) {
            // parse the arguments
            List<Expr> args = new ArrayList<>();
            if (!check(RIGHT_PAREN)) {
                // only try to parse arguments if the next character isn't ')'
                args.add(expression());
                while (match(COMMA)) {
                    if (args.size() >= 255) {
                        error(peek(), "Can't have more than 255 function arguments");
                    }
                    args.add(expression());
                }
            }
            Token paren = consume(RIGHT_PAREN, "Expect ')' after function call");
            return new Expr.Call(callee, paren, args);
        }

        return primary();
    }

    private Token consume(TokenType type, String message) {
        if (check(type)) return advance();
        throw error(peek(), message);
    }

    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }

    // method for error-handling when encountering a parsing error in a statement: discard the tokens in the
    // remainder of the statement, to avoid shotgunning the user with potentially correlated errors
    // and get to the next statement. This allows the user to discover as many independent errors as possible
    private void synchronize() {
        advance();
        while (!isAtEnd()) {
            // done synchronizing since we've reached the end of the statement and
            // are starting the parsing of a new statement
            if (previous().type == SEMICOLON) return;

            // these token signal the start of a new statement, so we start parsing the new statment
            switch(peek().type) {
                case CLASS:
                case FUN:
                case VAR:
                case FOR:
                case WHILE:
                case IF:
                case PRINT:
                case RETURN:
                    return;
            }

            // we're still parsing the problematic erroneous statement, keep discarding tokens until
            // we get to a new statement
            advance();
        }
    }

    private Expr primary() {
        if (match(FALSE)) return new Expr.Literal(false);
        if (match(TRUE)) return new Expr.Literal(true);
        if (match(NIL)) return new Expr.Literal(null);
        if (match(STRING, NUMBER)) return new Expr.Literal(previous().literal);
        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }
        if (match(IDENTIFIER)) return new Expr.Variable(previous());

        throw error(peek(), "Expect expression.");
    }
}
