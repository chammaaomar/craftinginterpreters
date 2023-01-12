#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum
{
    OP_CONSTANT,
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
uint8_t add_constant_to_chunk(Chunk *chunk, Value constant);

#endif