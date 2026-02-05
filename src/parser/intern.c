#include "intern.h"

static uint32_t hash_bytes(const char* data, size_t len) {
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619;
    }

    return hash;
}

void intern_init(StringInterner* si, ASTArena* arena) {
    si->capacity = 1024;
    si->count = 0;
    si->buckets = ast_arena_alloc_array(arena, si->capacity, sizeof(InternEntry*));

    for (size_t i = 0; i < si->capacity; i++) si->buckets[i] = NULL;
}

const char* intern_string(StringInterner* si, ASTArena* arena, const char* start, size_t len) {
    uint32_t hash = hash_bytes(start, len);
    size_t index = hash % si->capacity;

    InternEntry* entry = si->buckets[index];
    while (entry) {
        if (entry->length == len && memcmp(entry->string, start, len) == 0) {
            return entry->string;
        }
        
        entry = entry->next;
    }

    char* new_str = ast_arena_alloc_array(arena, len + 1, 1);
    memcpy(new_str, start, len);

    new_str[len] = '\0';

    InternEntry* new_entry = ast_arena_alloc_array(arena, 1, sizeof(InternEntry));

    new_entry->string = new_str;
    new_entry->length = len;
    new_entry->next = si->buckets[index];

    si->buckets[index] = new_entry;

    return new_str;
}
