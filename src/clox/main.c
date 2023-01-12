#include "common.h"
#include "chunk.h"
#include "debug.h"

// main currently plays the role of a yet-to-be-implemented frontend.
// the bytecode should be compiled from source code, but we don't have a scanner or parser
// yet, so we're pretending we have a clox program that generates the below instructions
// and then inspecting the produced bytecode to ensure we're building them correctly
int main(int argc, const char *argv[])
{
    Chunk chunk;
    init_chunk(&chunk);

    int constant = add_constant_to_chunk(&chunk, 1.2);
    write_chunk(&chunk, OP_CONSTANT, 123);
    write_chunk(&chunk, constant, 123);

    write_chunk(&chunk, OP_RETURN, 123);

    int second_constant = add_constant_to_chunk(&chunk, 2.4);
    write_chunk(&chunk, OP_CONSTANT, 127);
    write_chunk(&chunk, second_constant, 127);

    disassemble_chunk(&chunk, "test chunk");
    free_chunk(&chunk);

    return 0;
}