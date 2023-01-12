#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// abstract how we represent values in the C implementation of the bytecode and VM
// this way, if we change it, the change is encapsulated to this module

typedef double Value;

typedef struct
{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);

void print_value(Value value);

#endif