#include <stdlib.h>
#include "memory.h"

void *reallocate(void *pointer, size_t old_size, size_t new_size)
{
    if (new_size == 0)
    {
        // notice we don't need to pass how much memory to free
        // heap-allocated memory has additional bookkeeping bytes to help manage the memory
        free(pointer);
        return NULL;
    }

    // realloc is provided by the C standard library
    // if new_size > old_size it attempts to grow the memory allocation
    // if new_size < old_size it attempts to shrink the memory allocation, releasing memory
    // if old_size == 0, it allocates a new block of memory
    // if there isn't enough room after the existing allocation, it allocates a new block of memory,
    // frees the existing block, and copies over the bytes from the old allocation to the new one
    // freeing the existing one in the process: Precisely what we need to implement a dynamic array
    void *result = realloc(pointer, new_size);
    if (result == NULL)
    {
        // failed allocation
        exit(1);
    }
    return result;
}