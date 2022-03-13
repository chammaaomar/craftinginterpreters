package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();
    private int line = 1;
    private int current = 0;
    private int start = 0;

    private static final Map<String, TokenType> keywords;

    static {
        keywords = new HashMap<>();
        keywords.put("and", TokenType.AND);
        keywords.put("class", TokenType.CLASS);
        keywords.put("else", TokenType.ELSE);
        keywords.put("false", TokenType.FALSE);
        keywords.put("for", TokenType.FOR);
        keywords.put("fun", TokenType.FUN);
        keywords.put("if", TokenType.IF);
        keywords.put("nil", TokenType.NIL);
        keywords.put("or", TokenType.OR);
        keywords.put("print", TokenType.PRINT);
        keywords.put("return", TokenType.RETURN);
        keywords.put("super", TokenType.SUPER);
        keywords.put("this", TokenType.THIS);
        keywords.put("true", TokenType.TRUE);
        keywords.put("var", TokenType.VAR);
        keywords.put("while", TokenType.WHILE);
    }

    Scanner(String source) {
        this.source = source;
    }

    public List<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }

        tokens.add(new Token(TokenType.EOF, "", null, line));

        return tokens;
    }

    private boolean isAtEnd() {
        return current >= source.length();
    }


    private boolean match(char target) {
        // conditional advance: only consume the next character if it's what we're looking for
        if (isAtEnd()) return false;
        if (source.charAt(current) != target) return false;

        current++;
        return true;
    }

    private void scanToken() {
        char c = advance();
        switch (c) {
            // simple, one character tokens
            case '(': addToken(TokenType.LEFT_PAREN); break;
            case ')': addToken(TokenType.RIGHT_PAREN); break;
            case '[': addToken(TokenType.LEFT_BRACE); break;
            case ']': addToken(TokenType.RIGHT_BRACE); break;
            case ',': addToken(TokenType.COMMA); break;
            case '.': addToken(TokenType.DOT); break;
            case '-': addToken(TokenType.MINUS); break;
            case '+': addToken(TokenType.PLUS); break;
            case ';': addToken(TokenType.SEMICOLON); break;
            case '*': addToken(TokenType.STAR); break;

            // tokens that are part of 1-lexemes and 2-lexemes
            case '!': {
                addToken(match('=') ? TokenType.BANG_EQUAL : TokenType.BANG);
                break;
            }

            case '=': {
                addToken(match('=') ? TokenType.EQUAL_EQUAL : TokenType.EQUAL);
                break;
            }

            case '<': {
                addToken(match('=') ? TokenType.LESS_EQUAL : TokenType.LESS);
                break;
            }

            case '>': {
                addToken(match('=') ? TokenType.GREATER_EQUAL : TokenType.GREATER);
                break;
            }

            // carefully handle slashes since comments begin with slashes
            case '/': {
                if (match('/')) {
                    // comment
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    addToken(TokenType.SLASH);
                }
                break;
            }

            // skip over whitespace and newlines
            case '\n':  line++; break;
            case ' ':
            case '\r':
            case '\t': break;

            // string literals in the Lox language are either "-delimited or '-delimited
            case '"': string('"'); break;
            case '\'': string('\''); break;

            default:
                if (isDigit(c)) {
                    number();
                } else if (isAlpha(c)) {
                  identifier();
                } else {
                    Lox.error(line, "Unexpected character.");
                }
                break;
        }
    }

    private void string(char delimiter) {
        while (peek() != delimiter && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "Unterminated string.");
            return;
        }

        // move the scanner past the closing "
        advance();

        // reached end of string, capture it
        String value = source.substring(start + 1, current - 1);
        addToken(TokenType.STRING, value);
    }

    // scan and parse a number literal into a Java float
    // TODO: currently we only support decimal number literals
    // no scientific notation, hex, binary or octal numbers
    private void number() {
        // keep consuming the digits of the number
        while (isDigit(peek())) advance();

        if (peek() == '.' && isDigit(peekNext())) {
            // only consume the '.' if it has a following fractional part
            advance();

            while (isDigit(peek())) advance();
        }

        addToken(TokenType.NUMBER, Double.parseDouble(source.substring(start, current)));


    }

    // check if it is the second or later character of an identifier
    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }

    private void identifier() {
        while (isAlphaNumeric(peek())) advance();

        String text = source.substring(start, current);
        TokenType type = keywords.get(text);
        if (type == null) type = TokenType.IDENTIFIER;

        addToken(type);
    }

    private char advance() {
        return source.charAt(current++);
    }

    // one character lookahead; don't consume character
    private char peek() {
        if (isAtEnd()) return '\0';
        return source.charAt(current);
    }

    // two character lookahead; don't consume character
    // useful for parsing numeric literals
    private char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source.charAt(current + 1);
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    // check whether it's the first character of an identifier or a reserved keyword
    // which cannot be a number
    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c == '_');
    }

    private void addToken(TokenType type) {
        addToken(type, null);
    }

    private void addToken(TokenType type, Object literal) {
        String text = source.substring(start, current);
        tokens.add(new Token(type, text, literal, line));
    }
}
