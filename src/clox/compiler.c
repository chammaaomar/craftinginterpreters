#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef void (*ParseFn)(bool can_assign);

typedef struct
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct
{
    Token name;
    int depth;
} Local;

typedef struct
{
    Local locals[UINT8_COUNT];
    int local_count;
    int scope_depth;
} Compiler;

Parser parser;
Compiler *current = NULL;
Chunk *compiling_chunk;

static ParseRule *get_rule(TokenType type);
static void parse_precedence(Precedence precedence);
static void expression();
static void statement();
static void declaration();

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

static void init_compiler(Compiler *compiler)
{
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    current = compiler;
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

static uint8_t identifier_constant(Token *name)
{
    return make_constant(OBJ_VAL(copy_string(name->start, name->length)));
}

static void number(bool can_assign)
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void string(bool can_assign)
{
    emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static int resolve_local(Compiler *compiler, Token *name)
{
    // we walk starting from "last in" so that local variables shadow
    // variables from the surrounding scope
    for (int i = current->local_count - 1; i >= 0; i--)
    {
        Local *local = &current->locals[i];
        if (identifiers_equal(&local->name, name))
        {
            if (local->depth == -1)
            {
                error("Can't read a local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void named_variable(Token name, bool can_assign)
{
    uint8_t get_op, set_op;
    // the operand in the case of local variable expression or local variable assignment
    // is just the index into the compiler stack, which due to the structure of the language
    // is precisely the index into the VM's runtime stack.
    int arg = resolve_local(current, &name);

    if (arg != -1)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    else
    {
        arg = identifier_constant(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    if (can_assign && match(TOKEN_EQUAL))
    {
        expression();
        emit_bytes(get_op, (uint8_t)arg);
    }
    else
    {
        // global variables are late-bound, i.e., resolved at runtime, not compile time
        emit_bytes(set_op, (uint8_t)arg);
    }
}

static void variable(bool can_assign)
{
    named_variable(parser.previous, can_assign);
}

static void literal(bool can_assign)
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

// binary compiles a binary infix operator (+, -, *, /) and its right-hand-side operand
// the left-hand-side operand has already been compiled and its value has been pushed
// onto the stack, so this function only compiles the operator and r.h.s expression
// the expression can be any expression involing higer-precedence operators, since they "bind tigher"
static void binary(bool can_assign)
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

static void unary(bool can_assign)
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

static void grouping(bool can_assign)
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
// It is important that a statement leaves the stack unchanged, i.e., with no net pops or pushes
// because a program is made up of statements, and an long program shouldn't lead to a stack overflow
static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(OP_POP);
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope()
{
    while (current->local_count > 0 && current->locals[current->local_count - 1].depth == current->scope_depth)
    {
        // local variables are stored on the VM stack, not in the globals hash table, so we need to clear them
        emit_byte(OP_POP);
        current->local_count--;
    }
    current->scope_depth--;
}

static void block()
{
    while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF)
    {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void statement()
{
    if (match(TOKEN_PRINT))
    {
        print_statement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        // blocks and functions create local scope
        begin_scope();
        block();
        end_scope();
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

static void add_local(Token name)
{
    if (current->local_count == UINT8_COUNT)
    {
        error("Only a maximum of 256 local variables is supported.");
        return;
    }
    Local local = current->locals[current->local_count++];
    local.name = name;
    local.depth = -1;
}

static bool identifiers_equal(Token *a, Token *b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declare_variable()
{
    if (current->scope_depth == 0)
        return;
    Token *name = &parser.previous;
    for (int i = current->local_count - 1; i >= 0; i--)
    {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scope_depth)
        {
            // we've excited into the surrouding scope, and Lox supports variable shadowing
            // since scopes and their local variables are pushed unto the stack
            // so all variables of lower index are from the surrounding scope
            break;
        }

        if (identifiers_equal(&local->name, name))
        {
            error("A variable with this name already exists in the same scope.");
        }
    }
    add_local(*name);
}

static uint8_t parse_variable(const char *error_msg)
{
    consume(TOKEN_IDENTIFIER, error_msg);

    // local variable, we don't store these in the constants table
    // since at runtime, local variables aren't looked up by name
    // instead, they're looked up by their position in the VM stack
    declare_variable();
    if (current->scope_depth > 0)
        return 0;
    return identifier_constant(&parser.previous);
}

static void mark_initialized()
{
    current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void define_variable(uint8_t global)
{
    if (current->scope_depth > 0)
    {
        // mark the variable as available for use
        mark_initialized();
        return;
    }
    emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void var_declaration()
{
    // add the variable to the scope (whether local variable or global variable)
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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
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

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while (precedence <= get_rule(parser.current.type)->precedence)
    {
        // compile tokens as long as they're of higher precedence or "bind tighter"
        advance();
        ParseFn infix_rule = get_rule(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target.");
    }
}

bool compile(const char *source, Chunk *chunk)
{
    init_scanner(source);

    Compiler compiler;
    init_compiler(&compiler);

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
