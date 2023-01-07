#include <stdio.h>
#include "debug.h"

void disassemble_chunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
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

int disassemble_instruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    uint8_t instruction = chunk->code[offset];

    switch (instruction)
    {
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    default:
        printf("Unknown code %d\n", instruction);
        return offset + 1;
    }
}