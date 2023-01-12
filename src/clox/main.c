#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
    Chunk chunk;
    init_chunk(&chunk);
    write_chunk(&chunk, OP_RETURN);

    uint8_t constant = add_constant_to_chunk(&chunk, 1.2);
    write_chunk(&chunk, OP_CONSTANT);
    write_chunk(&chunk, constant);

    disassemble_chunk(&chunk, "test chunk");
    free_chunk(&chunk);

    return 0;
}