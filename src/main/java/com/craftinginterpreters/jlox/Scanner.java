package com.craftinginterpreters.jlox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();

    // start points to the first character in the lexeme currently being scanned
    private int start = 0;
    private int line = 1;
    private int current = 0;

    Scanner(String source) {
        this.source = source;
    }

    private static final Map<String, TokenType> keywords;

    static {
        keywords = new HashMap<>();
        keywords.put("and",    TokenType.AND);
        keywords.put("class",  TokenType.CLASS);
        keywords.put("else",   TokenType.ELSE);
        keywords.put("false",  TokenType.FALSE);
        keywords.put("for",    TokenType.FOR);
        keywords.put("fun",    TokenType.FUN);
        keywords.put("if",     TokenType.IF);
        keywords.put("nil",    TokenType.NIL);
        keywords.put("or",     TokenType.OR);
        keywords.put("print",  TokenType.PRINT);
        keywords.put("return", TokenType.RETURN);
        keywords.put("super",  TokenType.SUPER);
        keywords.put("this",   TokenType.THIS);
        keywords.put("true",   TokenType.TRUE);
        keywords.put("var",    TokenType.VAR);
        keywords.put("while",  TokenType.WHILE);
    }

    private boolean isAtEnd() {
        return current >= source.length();
    }

    private char advance() {
        return source.charAt(current++);
    }

    private void addToken(TokenType type) {
        addToken(type, null);
    }

    private void addToken(TokenType type, Object literal) {
        String text = source.substring(start, current);
        tokens.add(new Token(type, text, literal, line));
    }

    // match is a conditional advance, consuming the next character only if it matches the expected
    private boolean match(char expected) {
        if (isAtEnd()) return false;
        if (source.charAt(current) == expected) {
            current++;
            return true;
        } else {
            return false;
        }
    }

    // one-character lookahead
    private char peek() {
        if (isAtEnd()) return '\0';
        return source.charAt(current);
    }

    // two-character lookahead
    private char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source.charAt(current + 1);
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private boolean isAlpha(char c) {
        return c >= 'a' && c <= 'z' ||
                c >= 'A' && c <= 'Z' ||
                c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }

    private void identifier() {
        while (isAlphaNumeric(peek())) advance();
        String value = source.substring(start, current);
        TokenType type = keywords.get(value);
        if (type == null) {
            addToken(TokenType.IDENTIFIER);
        } else {
            addToken(type);
        }

    }

    private void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "Unterminated string.");
            return;
        }

        // to consume the terminating "
        advance();

        String text = source.substring(start + 1, current - 1);
        addToken(TokenType.STRING, text);
    }

    private void number() {
        while (isDigit(peek())) {
            advance();
        }

        // the decimal point is only consumed if followed by a number
        // we don't allow number literals like '1234.'
        if (peek() == '.' && isDigit(peekNext())) {
            // to consume the '.'
            advance();
            while (isDigit(peek())) {
                advance();
            }
        }

        Double value = Double.parseDouble(source.substring(start, current));
        addToken(TokenType.NUMBER, value);
    }

    List<Token> scanTokens() {
        while (!isAtEnd()) {
            scanToken();
        }
        tokens.add(new Token(TokenType.EOF, "", null, line));
        return tokens;
    }

    private void scanToken() {
        start = current;
        char c = advance();
        switch(c) {
            // Single-character lexemes
            case '(': addToken(TokenType.LEFT_PAREN); break;
            case ')': addToken(TokenType.RIGHT_PAREN); break;
            case '{': addToken(TokenType.LEFT_BRACE); break;
            case '}': addToken(TokenType.RIGHT_BRACE); break;
            case ',': addToken(TokenType.COMMA); break;
            case '.': addToken(TokenType.DOT); break;
            case '-': addToken(TokenType.MINUS); break;
            case '+': addToken(TokenType.PLUS); break;
            case ';': addToken(TokenType.SEMICOLON); break;
            case '*': addToken(TokenType.STAR); break;

            // Two-character lexemes
            case '!':
                addToken(match('=') ? TokenType.BANG_EQUAL : TokenType.BANG);
                break;

            case '=':
                addToken(match('=') ? TokenType.EQUAL_EQUAL : TokenType.EQUAL);
                break;

            case '>':
                addToken(match('=') ? TokenType.GREATER_EQUAL : TokenType.GREATER);
                break;

            case '<':
                addToken(match('=') ? TokenType.LESS_EQUAL : TokenType.LESS);
                break;

            case '/':
                if (match('/')) {
                    while (peek() != '\n' && !isAtEnd()) {
                        advance();
                    }
                } else {
                    addToken(TokenType.SLASH);
                }
                break;

            case '"': string(); break;



                // ignore whitespace
            case ' ':
            case '\t':
            case '\r':
                break;

            case '\n':
                line++; break;

            default:
                if (isDigit(c)) {
                    number(); break;
                } else if (isAlpha(c)) {
                    identifier();
                } else {
                    Lox.error(line, "Unexpected character.");
                    break;
                }
        }
    }

}
