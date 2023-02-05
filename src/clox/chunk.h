#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT,
    // constant long instruction allows us to load constants with index in the constant
    // pool that doesn't fit into a single byte. A single byte only allows us 255 constants
    // in a chunk (program), whereas with this instruction, we allow 3 bytes for the index.
    OP_CONSTANT_LONG,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_RETURN,
} OpCode;

// Chunk represents a clox program, which is a dynamic array of opcodes / bytecode instructions
// and a dynamic array of data acted on by those instructions
typedef struct
{
    int capacity;
    int count;
    uint8_t *code;
    ValueArray constants;
    int *lines;
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
void free_chunk(Chunk *chunk);
int add_constant_to_chunk(Chunk *chunk, Value constant);
void write_constant(Chunk *chunk, Value value, int line);

#endif