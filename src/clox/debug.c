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

static int byte_instruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

int constant_long_instruction(const char *name, Chunk *chunk, int offset)
{
    uint8_t constant_ptr_lowest_byte = chunk->code[offset + 1];
    uint8_t constant_ptr_middle_byte = chunk->code[offset + 1];
    uint8_t constant_ptr_highest_byte = chunk->code[offset + 1];
    int constant_ptr = constant_ptr_lowest_byte | constant_ptr_middle_byte | constant_ptr_highest_byte;
    printf("%-16s %4d '", name, constant_ptr);
    print_value(chunk->constants.values[constant_ptr]);
    printf("'\n");
    return offset + 4;
}

int disassemble_instruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    {
        // instruction on same line as previous instruction
        printf("   | ");
    }
    else
    {
        // instruction on new line
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];

    switch (instruction)
    {
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    case OP_POP:
        return simple_instruction("OP_POP", offset);
    case OP_CONSTANT:
        return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_NEGATE:
        return simple_instruction("OP_NEGATE", offset);
    // the arithmetic operators do take operands, so "+" has two operands
    // but the arithmetic bytecode instructions do NOT, since they just read
    // their operands off the stack
    case OP_ADD:
        return simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simple_instruction("OP_DIVIDE", offset);
    case OP_TRUE:
        return simple_instruction("OP_TRUE", offset);
    case OP_FALSE:
        return simple_instruction("OP_FALSE", offset);
    case OP_NIL:
        return simple_instruction("OP_NIL", offset);
    case OP_NOT:
        return simple_instruction("OP_NOT", offset);
    case OP_EQUAL:
        return simple_instruction("OP_EQUAL", offset);
    case OP_GREATER:
        return simple_instruction("OP_GREATER", offset);
    case OP_LESS:
        return simple_instruction("OP_LESS", offset);
    case OP_PRINT:
        return simple_instruction("OP_PRINT", offset);
    case OP_DEFINE_GLOBAL:
        return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
        return constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OP_GET_LOCAL:
        return byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_GLOBAL:
        return constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OP_SET_LOCAL:
        return byte_instruction("OP_SET_LOCAL", chunk, offset);
    default:
        printf("Unknown code %d\n", instruction);
        return offset + 1;
    }
}