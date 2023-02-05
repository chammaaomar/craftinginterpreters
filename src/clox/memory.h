#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

// OK to waste some space for smaller arrays, at the benefit of having to allocate and copy
// fewer times when initially growing
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity)*2)

// some generics / template programming on types
#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type *)reallocate(pointer, sizeof(type) * (old_count), sizeof(type) * (new_count))

void *reallocate(void *pointer, size_t old_size, size_t new_size);

#define FREE_ARRAY(type, pointer, capacity) \
    reallocate(pointer, sizeof(type) * (capacity), 0);

#define ALLOCATE(type, count) \
    (type *)reallocate(NULL, 0, sizeof(type) * (count))

#endif