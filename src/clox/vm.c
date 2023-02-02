#include <stdio.h>
#include "vm.h"
#include "debug.h"
#include "compiler.h"

// vm is a single, global instance
VM vm;

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

void init_vm()
{
    reset_stack();
}

void free_vm()
{
}

void push(Value value)
{
    *(vm.stack_top++) = value;
}

Value pop()
{
    return *(--vm.stack_top);
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_BYTE() | READ_BYTE() | READ_BYTE()])
// BINARY_OP uses a block to ensure that the statements executed have the same scope.
// notice that in Lox, we define order of evaluation from left to right
// so for example if we want to calculate expr_a + expr_b, we evaluate
// first expr_a, push it on the stack, then evaluate expr_b and push it on the stack
// so expr_a is actually popped later, in a first-in-last-out order
// also ops like + isn't first class, but the C preprocessor only cares about text tokens
// not C language tokens
#define BINARY_OP(op)        \
    {                        \
        Value right = pop(); \
        Value left = pop();  \
        push(left op right); \
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
            print_value(pop());
            printf("\n");
            return INTERPRET_OK;
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
        case OP_NEGATE:
        {
            push(-pop());
            break;
        }
        case OP_ADD:
            BINARY_OP(+)
            break;
        case OP_SUBTRACT:
            BINARY_OP(-)
            break;
        case OP_MULTIPLY:
            BINARY_OP(*)
            break;
        case OP_DIVIDE:
            BINARY_OP(/)
            break;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
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