#include "common.h"
#include "chunk.h"
#include "vm.h"
#include "debug.h"

// main currently plays the role of a yet-to-be-implemented frontend.
// the bytecode should be compiled from source code, but we don't have a scanner or parser
// yet, so we're pretending we have a clox program that generates the below instructions
// and then inspecting the produced bytecode to ensure we're building them correctly
int main(int argc, const char *argv[])
{

    init_vm();
    Chunk chunk;
    init_chunk(&chunk);

    int constant = add_constant_to_chunk(&chunk, 1.2);
    write_chunk(&chunk, OP_CONSTANT, 123);
    write_chunk(&chunk, constant, 123);

    int second_constant = add_constant_to_chunk(&chunk, 3.4);
    write_chunk(&chunk, OP_CONSTANT, 123);
    write_chunk(&chunk, second_constant, 123);

    write_chunk(&chunk, OP_ADD, 123);

    int third_constant = add_constant_to_chunk(&chunk, 5.6);
    write_chunk(&chunk, OP_CONSTANT, 123);
    write_chunk(&chunk, third_constant, 123);

    write_chunk(&chunk, OP_DIVIDE, 123);

    write_chunk(&chunk, OP_NEGATE, 123);

    write_chunk(&chunk, OP_RETURN, 123);

    interpret(&chunk);
    free_chunk(&chunk);
    free_vm();

    return 0;
}