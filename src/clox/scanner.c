#include <string.h>
#include "scanner.h"
#include "common.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

void init_scanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool is_at_end()
{
    return *scanner.current == '\0';
}

static Token make_token(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

static Token error_token(const char *message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = scanner.line;

    return token;
}

static char advance()
{
    return *scanner.current++;
}

static char peek()
{
    return *scanner.current;
}

static char peek_next()
{
    if (is_at_end())
        return '\0';
    return scanner.current[1];
}

static void skip_whitespace()
{
    for (;;)
    {
        char c = peek();
        switch (c)
        {
        case '\t':
        case '\r':
        case ' ':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peek_next() == '/')
            {
                // we're in a comment; keep consuming and throwing away tokens until newline
                // because we increment the line counter at end of line
                while (peek() != '\n' && !is_at_end())
                    advance();
            }
            else
            {
                // not whitespace, stop skipping and parse the token
                return;
            }
        default:
            return;
        }
    }
}

// match consumes the next token, only if it matches the expected token
// especially useful for parsing lexemes that begin with the same character
static char match(char expected)
{
    if (is_at_end())
        return false;
    if (*scanner.current != expected)
        return false;

    advance();
    return true;
}

static Token string()
{
    while (peek() != '"' && !is_at_end())
    {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }

    if (is_at_end())
    {
        return error_token("Unterminated string.");
    }

    // consume the closing quote
    advance();

    return make_token(TOKEN_STRING);
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_alphanumeric(char c)
{
    return is_digit(c) || is_alpha(c);
}

static Token number()
{
    while (is_digit(peek()))
    {
        advance();
    }

    // look for fractional part
    if (peek() == '.' && is_digit(peek_next()))
    {
        advance();
        // consume fractional part
        while (is_digit(peek()))
            advance();
    }

    return make_token(TOKEN_NUMBER);
}

static TokenType check_keyword(int start, int length, const char *rest, TokenType type)
{
    if ((scanner.current - scanner.start == start + length) && memcmp(scanner.start + start, rest, length) == 0)
    {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifer_type()
{
    switch (scanner.start[0])
    {
    case 'a':
        return check_keyword(1, 2, "nd", TOKEN_AND);
    case 'c':
        return check_keyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
        return check_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'a':
                return check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'o':
                return check_keyword(2, 1, "r", TOKEN_FOR);
            case 'u':
                return check_keyword(2, 1, "n", TOKEN_FUN);
            }
        }
    case 'i':
        return check_keyword(1, 1, "f", TOKEN_IF);
    case 'n':
        return check_keyword(1, 2, "il", TOKEN_NIL);
    case 'o':
        return check_keyword(1, 1, "r", TOKEN_OR);
    case 'p':
        return check_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return check_keyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'h':
                return check_keyword(2, 2, "is", TOKEN_THIS);
            case 'r':
                return check_keyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
    case 'v':
        return check_keyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
        return check_keyword(1, 4, "hile", TOKEN_WHILE);
    default:
        return TOKEN_IDENTIFIER;
    }
}

Token identifier()
{
    while (is_alphanumeric(peek()))
    {
        advance();
    }

    return make_token(identifer_type());
}

// scan_token produces the next token according to Lox's lexical grammar.
// This lazily produces the next token (on demand), as opposed to the Java
// implementation in which all the tokens are eagerly produced (scanTokens)
// this is done so we don't have to manually manage the memory of dynamic arrays
// and all the produced token structs
Token scan_token()
{
    // ensure the produced token is a meaningful token in the lexical grammar
    skip_whitespace();
    // since each call starts at a new token
    scanner.start = scanner.current;

    char c = advance();

    if (is_alpha(c))
        return identifier();

    if (is_digit(c))
        return number();

    switch (c)
    {
    // single-character tokens
    case '(':
        return make_token(TOKEN_LEFT_PAREN);
    case ')':
        return make_token(TOKEN_RIGHT_PAREN);
    case '{':
        return make_token(TOKEN_LEFT_BRACE);
    case '}':
        return make_token(TOKEN_RIGHT_BRACE);
    case '+':
        return make_token(TOKEN_PLUS);
    case '-':
        return make_token(TOKEN_MINUS);
    case '*':
        return make_token(TOKEN_STAR);
    case '/':
        return make_token(TOKEN_SLASH);
    case ',':
        return make_token(TOKEN_COMMA);
    case '.':
        return make_token(TOKEN_DOT);
    case ';':
        return make_token(TOKEN_SEMICOLON);
    // two- or one-character tokens, depending on the next character (one-character lookahead grammar)
    case '>':
        return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '<':
        return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '=':
        return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '!':
        return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '"':
        return string();
    }

    if (is_at_end())
        return make_token(TOKEN_EOF);

    return error_token("Unexpected character.");
}