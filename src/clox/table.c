#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(Table *table)
{
    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;
}

void free_table(Table *table)
{
    FREE_ARRAY(Entry, table->entries, table->capacity);
    init_table(table);
}

// find_entry finds which bucket a key-value pair should go into and uses linear
// probing for resolving hash collisions. This has no risk of infinite loop, i.e.,
// a collision at every bucket, because we take care to grow the array prior to calling
// this procedure if its count/capacity ratio exceeds the load factor (TABLE_MAX_LOAD)
Entry *find_entry(Entry *entries, int capacity, ObjString *key)
{
    uint32_t index = key->hash % capacity;
    Entry *tombstone = NULL;
    for (;;)
    {
        Entry *entry = &entries[index];
        if (entry->key == key)
        {
            return entry;
        }
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value))
            {
                // empty entry
                // return tombstone if we've encountered one, so we can reuse it and not waste the space
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                // tombstone
                if (tombstone == NULL)
                    tombstone = entry;
            }
        }
        index = (index + 1) % capacity;
    }
}

// adjust_capacity is used when adding entries to the hash table: It allocates a new
// array of size "capacity" and copies over the entries from "table", minus the tombstone
// entries
static void adjust_capacity(Table *table, int capacity)
{
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    // re-calculate the count, because we're not copying over tombstones, since we're re-creating the
    // linear probe sequences anyway, so the tombstones add no value
    table->count = 0;

    for (int i = 0; i < table->capacity; i++)
    {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL)
            continue;

        Entry *dst = find_entry(entries, capacity, entry->key);
        dst->key = entry->key;
        dst->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

bool table_set(Table *table, ObjString *key, Value value)
{
    // ensure the underlying storage array is big enough to accomodate a new insert
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }
    Entry *entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == NULL;
    // only increment if filling an actually empty entry, not tombstone
    if (is_new_key && IS_NIL(entry->value))
        table->count++;

    entry->key = key;
    entry->value = value;
    return is_new_key;
}

// table_add_all copies all the entries from hash "from" to hash "to"
void table_add_all(Table *from, Table *to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        Entry entry = from->entries[i];
        if (entry.key != NULL)
        {
            table_set(to, entry.key, entry.value);
        }
    }
}

// table_get gets the value from "table" corresponding to "key"
// if it's in the table, and points the "value" pointer to it
// otherwise it returns false
bool table_get(Table *table, ObjString *key, Value *value)
{
    if (table->count == 0)
        // if the table has no item in it, then the entries array is empty and uninitialized
        // and we don't want to traverse it
        return false;
    Entry *entry = find_entry(table->entries, table->capacity, key);
    if (entry->key != NULL)
    {
        *value = entry->value;
        return true;
    }
    else
    {
        return false;
    }
}

// table_delete "deletes" an entry be replacing it with a tombstone.
// This is important for linear probing / open addressing hash collision resolution
// since we want to ensure that collision chains aren't broken, which would lead to
// unreachable entries and an unrealiable hash table
// details: tombstone entries are considered entries in terms of the "count", so we don't decrement
// the count on deletion
bool table_delete(Table *table, ObjString *key)
{
    if (table->count == 0)
        return false;
    Entry *entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return false;

    // tombstone
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

ObjString *table_find_string(Table *table, const char *chars, int length, uint32_t hash)
{
    if (table->count == 0)
        return NULL;
    uint32_t index = hash % table->capacity;

    for (;;)
    {
        Entry *entry = &table->entries[index];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value))
            {
                return NULL;
            }
        }
        else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
        {
            return entry->key;
        }
        index = (index + 1) % table->capacity;
    }
}