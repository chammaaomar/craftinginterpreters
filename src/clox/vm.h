#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct
{
    Chunk *chunk;
    // Instruction Pointer points to the instruction about to be executed
    uint8_t *ip;
    Value stack[STACK_MAX];
    // stack_top points to the array element just past the top array element in the stack.
    // Thus we can indicate the stack is empty by pointing at 0
    Value *stack_top;
} VM;

typedef enum
{
    INTERPRET_OK,
    // compiler detects static (syntatic and semantic) errors
    INTERPRET_COMPILE_ERROR,
    // VM detects runtime errors
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void init_vm();
void free_vm();
InterpretResult interpret(Chunk *chunk);
void push(Value value);
Value pop();

#endif