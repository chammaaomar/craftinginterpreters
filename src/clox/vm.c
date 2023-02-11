#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"

// vm is a single, global instance
VM vm;

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

void init_vm()
{
    reset_stack();
    vm.objects = NULL;
    init_table(&vm.strings);
    init_table(&vm.globals);
}

void free_vm()
{
    free_table(&vm.globals);
    free_table(&vm.strings);
    free_objects();
}

static void runtime_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack();
}

void push(Value value)
{
    *(vm.stack_top++) = value;
}

Value pop()
{
    return *(--vm.stack_top);
}

Value peek(int distance)
{
    return vm.stack_top[-1 - distance];
}

static bool is_falsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString *b = AS_STRING(pop());
    ObjString *a = AS_STRING(pop());

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = take_string(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_STRING() (AS_STRING(READ_CONSTANT()))
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_BYTE() | READ_BYTE() | READ_BYTE()])
// BINARY_OP uses a block to ensure that the statements executed have the same scope.
// notice that in Lox, we define order of evaluation from left to right
// so for example if we want to calculate expr_a + expr_b, we evaluate
// first expr_a, push it on the stack, then evaluate expr_b and push it on the stack
// so expr_a is actually popped later, in a first-in-last-out order
// also ops like + isn't first class, but the C preprocessor only cares about text tokens
// not C language tokens
// this macro only supports Lox numbers on the stack, but can return different Lox types
#define BINARY_OP(value_Type, op)                       \
    {                                                   \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
        {                                               \
            runtime_error("Operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR;             \
        }                                               \
        double right = AS_NUMBER(pop());                \
        double left = AS_NUMBER(pop());                 \
        push(value_Type(left op right));                \
    }

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        // contents of the stack
        printf("          ");
        for (Value *slot = vm.stack; slot < vm.stack_top; slot++)
        {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_RETURN:
        {
            return INTERPRET_OK;
        }
        case OP_POP:
        {
            // not useful thus far because our expressions don't have side effects
            // function calls are examples of expressions with side effects
            pop();
            break;
        }
        case OP_CONSTANT:
        {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OP_CONSTANT_LONG:
        {
            Value constant = READ_CONSTANT_LONG();
            break;
        }
        case OP_TRUE:
            push(BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(BOOL_VAL(false));
            break;
        case OP_NIL:
            push(NIL_VAL);
            break;
        case OP_NEGATE:
        {
            if (!IS_NUMBER(peek(0)))
            {
                runtime_error("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            else
            {

                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
        }
        case OP_NOT:
            push(BOOL_VAL(is_falsey(pop())));
            break;
        case OP_ADD:
        {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
            {
                concatenate();
            }
            else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
            {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            }
            else
            {
                runtime_error("Operands must be both strings or numbers");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -)
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *)
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /)
            break;
        case OP_EQUAL:
        {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(value_equals(a, b)));
            break;
        }
        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;
        case OP_PRINT:
            print_value(pop());
            printf("\n");
            break;
        case OP_DEFINE_GLOBAL:
        {
            ObjString *global_name = READ_STRING();
            // we're doing peek(0) then pop() as opposed to just pop() for a reason
            // regarding garbage collection that I don't really understand
            table_set(&vm.globals, global_name, peek(0));
            pop();
            break;
        }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_CONSTANT_LONG
#undef BINARY_OP
}

InterpretResult interpret(const char *source)
{
    Chunk chunk;
    init_chunk(&chunk);

    if (!compile(source, &chunk))
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    free_chunk(&chunk);

    return result;
}