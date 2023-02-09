#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type) \
    (type *)allocate_object(sizeof(type), object_type)

static Obj *allocate_object(size_t size, ObjType type)
{
    Obj *object = (Obj *)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static ObjString *allocate_string(char *chars, int length, uint32_t hash)
{
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    table_set(&vm.strings, string, NIL_VAL);

    return string;
}

// FNV-1a hash function
uint32_t hash_string(const char *chars, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= (uint8_t)chars[i];
        hash *= 16777619;
    }
    return hash;
}

// copy_string copies chars, which points into the user's source code, into heap-allocated memory
// since it cannot take ownership of the user's source code, and then it allocates a Lox string
// pointing to the heap-allocated chars we just copied
ObjString *copy_string(const char *chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL)
        return interned;

    // new unique string, add it to the collection of interned strings
    char *heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length, hash);
}

// take_string takes ownership of the heap-allocated character array that's passed in, and allocates
// a Lox string pointing to chars
ObjString *take_string(char *chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    ObjString *interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL)
    {
        // ownership is passed to this function, and it no longer needs the passed in string, so just free it up
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    // new unique string, add it to the collection of interned strings
    return allocate_string(chars, length, hash);
}

void print_object(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}