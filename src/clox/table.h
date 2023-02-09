#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct
{
    ObjString *key;
    Value value;
} Entry;

// Table is the underlying data structure for a hash table. It's just a dynamically
// resizable array
typedef struct
{
    // count counts the entries with values and tombstone entries, which are an implementation detail
    // enabling lazy deletion
    int count;
    int capacity;
    Entry *entries;
} Table;

void init_table(Table *table);
void free_table(Table *table);
bool table_set(Table *table, ObjString *key, Value value);
void table_add_all(Table *from, Table *to);
bool table_get(Table *table, ObjString *key, Value *value);
bool table_delete(Table *table, ObjString *key);
ObjString *table_find_string(Table *table, const char *chars, int length, uint32_t hash);
#endif