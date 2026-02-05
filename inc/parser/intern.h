#ifndef INTERN_H
#define INTERN_H

#include "ast_buffer.h"
#include <string.h>

typedef struct InternEntry {
    const char* string;
    size_t length;
    struct InternEntry* next;
} InternEntry;

typedef struct {
    InternEntry** buckets;
    size_t capacity;
    size_t count;
} StringInterner;

void intern_init(StringInterner* si, ASTArena* arena);
const char* intern_string(StringInterner* si, ASTArena* arena, const char* start, size_t len);

#endif
