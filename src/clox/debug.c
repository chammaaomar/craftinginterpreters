#include <stdio.h>
#include "debug.h"

void disassemble_chunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
    // iterate in this interesting way because instructions aren't of uniform size
    // some are 1-byte long others are longer
    {
        // instructions can have different sizes, so delegate incrementing to disassemble_instruction
        offset = disassemble_instruction(chunk, offset);
    }
}

int simple_instruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int constant_instruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t constant_ptr = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant_ptr);
    print_value(chunk->constants.values[constant_ptr]);
    printf("'\n");
    return offset + 2;
}

int disassemble_instruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    uint8_t instruction = chunk->code[offset];

    switch (instruction)
    {
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    case OP_CONSTANT:
        return constant_instruction("OP_CONSTANT", chunk, offset);
    default:
        printf("Unknown code %d\n", instruction);
        return offset + 1;
    }
}