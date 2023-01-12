#include "chunk.h"
#include "memory.h"

// pass by pointer since we need to modify it
void init_chunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    init_value_array(&chunk->constants);
}

void write_chunk(Chunk *chunk, uint8_t byte)
{
    if (chunk->count >= chunk->capacity)
    {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}

void free_chunk(Chunk *chunk)
{
    free_value_array(&chunk->constants);
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    init_chunk(chunk);
}

// adds a constant to the constant dynamic array of chunk (which is a representation of a clox program; instructions and data)
// this will be called by the VM once implemented
// returns the index at which the constant was added for later easy access
uint8_t add_constant_to_chunk(Chunk *chunk, Value constant)
{
    write_value_array(&chunk->constants, constant);
    return chunk->constants.count - 1;
}