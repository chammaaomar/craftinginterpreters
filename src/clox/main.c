#include "common.h"
#include "chunk.h"

int main(int argc, const char *argv[])
{
    Chunk chunk;
    init_chunk(&chunk);
    write_chunk(&chunk, OP_RETURN);
    free_chunk(&chunk);

    return 0;
}