#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

typedef enum
{
    OP_RETURN,
} OpCode;

// dynamic array representing a sequence of opcodes / operations each 1-byte long
typedef struct
{
    int capacity;
    int count;
    uint8_t *code;
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte);
void free_chunk(Chunk *chunk);

#endif