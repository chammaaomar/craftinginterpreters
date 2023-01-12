#include "chunk.h"
#include "memory.h"

// pass by pointer since we need to modify it
void init_chunk(Chunk *chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    init_value_array(&chunk->constants);
}

void write_chunk(Chunk *chunk, uint8_t byte, int line)
{
    if (chunk->count >= chunk->capacity)
    {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_capacity, chunk->capacity);
    }

    chunk->lines[chunk->count] = line;
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

void free_chunk(Chunk *chunk)
{
    free_value_array(&chunk->constants);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    init_chunk(chunk);
}

// adds a constant to the constant dynamic array of chunk (which is a representation of a clox program; instructions and data)
// this will be called by the VM once implemented
// returns the index at which the constant was added for later easy access
int add_constant_to_chunk(Chunk *chunk, Value constant)
{
    write_value_array(&chunk->constants, constant);
    return chunk->constants.count - 1;
}

void write_constant(Chunk *chunk, Value value, int line)
{
    if (chunk->constants.count < 0b11111111)
    {
        write_chunk(chunk, OP_CONSTANT, line);
        // in this branch, constant_ptr, the index into the constants pool
        // fits into a single byte
        int constant_ptr = add_constant_to_chunk(chunk, value);
        write_chunk(chunk, constant_ptr, line);
    }
    else
    {
        // the index into the constants pool doesn't fit into a single byte
        // let's fit it into 24-bits or 3 bytes
        write_chunk(chunk, OP_CONSTANT_LONG, line);
        int constant_ptr = add_constant_to_chunk(chunk, value);
        write_chunk(chunk, constant_ptr & 0b11111111, line);
        write_chunk(chunk, constant_ptr & (0b11111111 << 8), line);
        write_chunk(chunk, constant_ptr & (0b11111111 << 16), line);
    }
}