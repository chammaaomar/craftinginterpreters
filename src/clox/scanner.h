#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,
    // One or two character tokens.
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    // Literals.
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    // Keywords.
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,

    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

// Token represents a token produced by the scanner. For example, in the statement
// "print 1+2;" the scanner produces the following tokens
// "print", "1", "+", "2", ";", EOF
// As opposed the Java implementation of Lox, each Token doesn't hold its own lexeme,
// instead it points into the original source code via "start" and "length". This way
// we don't need to dynamically manage the memory, and just ensure that the original
// source data structure outlives all the lexemes, i.e., isn't freed until they're all
// interpreted by the VM.
typedef struct
{
    const char *start;
    int length;
    TokenType type;
    int line;
} Token;

void init_scanner(const char *source);
Token scan_token();

#endif