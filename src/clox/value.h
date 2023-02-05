#ifndef clox_value_h
#define clox_value_h

#include "common.h"

// abstract how we represent values in the C implementation of the bytecode and VM
// this way, if we change it, the change is encapsulated to this module

typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        bool boolean;
        double number;
    } as;
} Value;

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_NIL(value) ((value).type == VAL_NIL)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

typedef struct
{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);
bool value_equals(Value a, Value b);

void print_value(Value value);

#endif