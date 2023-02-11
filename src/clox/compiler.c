#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct
{
    Token previous;
    Token current;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // ==
    PREC_COMPARISON, // >, >=, <, <=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // () .
    PREC_PRIMARY     // literals and identifiers
} Precedence;

typedef void (*ParseFn)();

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compiling_chunk;

static void error_at(Token *token, const char *message)
{
    if (parser.panic_mode)
        return;
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Lexical error
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void error_at_current(const char *message)
{
    error_at(&parser.current, message);
}

static void error(const char *message)
{
    error_at(&parser.previous, message);
}

static void advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR)
            break;

        error_at_current(parser.current.start);
    }
}

static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }

    error_at_current(message);
}

Chunk *current_chunk()
{
    return compiling_chunk;
}

static void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

static bool match(TokenType type)
{
    if (parser.current.type == type)
    {
        advance();
        return true;
    }

    return false;
}

static void emit_return()
{
    emit_byte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
    {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

static void end_compiler()
{
    emit_return();
}

static uint8_t make_constant(Value value)
{
    int constant = add_constant_to_chunk(compiling_chunk, value);
    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emit_constant(Value value)
{
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void string()
{
    emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void literal()
{
    Token token = parser.previous;
    switch (token.type)
    {
    case TOKEN_TRUE:
        emit_byte(OP_TRUE);
        break;
    case TOKEN_FALSE:
        emit_byte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emit_byte(OP_NIL);
        break;
    default:
        return;
    }
}

static ParseRule *get_rule(TokenType type);
static void parse_precedence(Precedence precedence);
static void expression();
static void statement();
static void declaration();

// binary compiles a binary infix operator (+, -, *, /) and its right-hand-side operand
// the left-hand-side operand has already been compiled and its value has been pushed
// onto the stack, so this function only compiles the operator and r.h.s expression
// the expression can be any expression involing higer-precedence operators, since they "bind tigher"
static void binary()
{
    TokenType operator_type = parser.previous.type;
    ParseRule *rule = get_rule(operator_type);
    // binary operators are left-associative, so the following expression: 1 + 2 + 3 + 4
    // should be evaluated as ((1+2) + 3) + 4
    // so only the l.h.s operand contains the binary operator
    // we ensure that by parsing the r.h.s expression using operators with _strictly_ higher precedence
    // not the same precedence
    parse_precedence((Precedence)(rule->precedence + 1));

    switch (operator_type)
    {
    case TOKEN_PLUS:
        emit_byte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emit_byte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emit_byte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emit_byte(OP_DIVIDE);
        break;
    case TOKEN_EQUAL_EQUAL:
        emit_byte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emit_byte(OP_GREATER);
        break;
    case TOKEN_LESS:
        emit_byte(OP_LESS);
        break;
    case TOKEN_BANG_EQUAL:
        emit_bytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_GREATER_EQUAL:
        emit_bytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS_EQUAL:
        emit_bytes(OP_GREATER, OP_NOT);
        break;
    default:
        return;
    }
}

static void unary()
{
    // we've already consumed the prefix unary operator, so it's in the previous token
    TokenType operator_type = parser.previous.type;

    // compile the expression, i.e., emit its bytecode
    // the operand appears ahead of the operator, since it already needs to be on the stack
    // so the operator pops it, applies negation or logical negation, and pushes it back on the stack
    parse_precedence(PREC_UNARY);

    switch (operator_type)
    {
    case TOKEN_MINUS:
        emit_byte(OP_NEGATE);
        break;
    case TOKEN_BANG:
        emit_byte(OP_NOT);
        break;
    default:
        return;
    }
}

static void grouping()
{
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void expression()
{
    parse_precedence(PREC_ASSIGNMENT);
}

static void print_statement()
{
    // puts the result of evaluating the expression on the stack
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(OP_PRINT);
}

// an expression statement evaluates an expression for its side effect, and discards its result
// _i.e._, pops it off the stack, since statements must always leave the stack unchanged, with no additions
// or deletions. An example of expression statements are function calls, since they produce values but it
// can be discarded, and assignment expressions.
static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(OP_POP);
}

static void statement()
{
    if (match(TOKEN_PRINT))
    {
        print_statement();
    }
    else
    {
        expression_statement();
    }
}

// keep discarding tokens until we get to the next statement
static void synchronize()
{
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.previous.type == TOKEN_SEMICOLON)
        {
            // done with error synchronization, since we reached a statement boundary
            return;
        }
        switch (parser.current.type)
        {
        TOKEN_IF:
        TOKEN_FOR:
        TOKEN_WHILE:
        TOKEN_CLASS:
        TOKEN_FUN:
        TOKEN_VAR:
        TOKEN_PRINT:
        TOKEN_RETURN:
            return;
        default:;
            // do nothing, essentially discarding the tokens until we get to a statement boundary
            // so we don't shotgun the user with cascading error messages that are mostly correlated and noisy
        }
        advance();
    }
}

static uint8_t identifier_constant(Token *name)
{
    return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
}

static uint8_t parse_variable(const char *error_msg)
{
    consume(TOKEN_IDENTIFIER, error_msg);
    return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global)
{
    emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void var_declaration()
{
    uint8_t global = parse_variable("Expect variable name.");

    if (match(TOKEN_EQUAL))
    {
        expression();
    }
    else
    {
        emit_byte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    define_variable(global);
}

static void declaration()
{
    // TODO: Add variable declaration
    if (match(TOKEN_VAR))
    {
        var_declaration();
    }
    else
    {
        statement();
    }

    if (parser.panic_mode)
        synchronize();
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// get_rule exists solely as a layer of indirection to break a declaration cycle
// binary cannot access the rules[] directly, since it refers to binary, so the
// declaration cycle is broken via get_rule
ParseRule *get_rule(TokenType type)
{
    return &rules[type];
}

static void parse_precedence(Precedence precedence)
{
    advance();
    // parse a prefix expression
    // all prefix precedence operators in Lox have the same precedence level
    ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL)
    {
        error("Expect expression");
        return;
    }

    prefix_rule();

    while (precedence <= get_rule(parser.current.type)->precedence)
    {
        // compile tokens as long as they're of higher precedence or "bind tighter"
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule();
    }
}

bool compile(const char *source, Chunk *chunk)
{
    init_scanner(source);

    compiling_chunk = chunk;

    parser.had_error = false;
    parser.panic_mode = false;

    int line = -1;

    advance();
    while (!match(TOKEN_EOF))
    {
        declaration();
    }
    end_compiler();

    return !parser.had_error;
}
