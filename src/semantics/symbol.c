#include "symbol.h"

static size_t hash_name(const char* name, size_t capacity) {
    size_t hash = 5381;
    int c;
    
    while ((c = *name++)) hash = ((hash << 5) + hash) + (size_t)c;

    return hash % capacity;
}

Scope* scope_new(ASTArena* arena, Scope* parent) {
    Scope* s = ast_arena_alloc_array(arena, 1, sizeof(Scope));

    s->capacity = 32;
    s->buckets = ast_arena_alloc_array(arena, s->capacity, sizeof(Symbol*));

    for (size_t i = 0; i < s->capacity; i++) s->buckets[i] = NULL;
    s->parent = parent;
    
    return s;
}

void scope_define(ASTArena* arena, Scope* s, const char* name, SymbolKind kind, AST* node) {
    size_t index = hash_name(name, s->capacity);
    
    Symbol* sym = ast_arena_alloc_array(arena, 1, sizeof(Symbol));
    
    sym->name = name;
    sym->kind = kind;
    sym->decl_node = node;
    
    sym->next = s->buckets[index];
    s->buckets[index] = sym;
}

Symbol* scope_lookup(Scope* s, const char* name) {
    while (s != NULL) {
        size_t index = hash_name(name, s->capacity);
        Symbol* curr = s->buckets[index];
        
        while (curr != NULL) {
            if (curr->name == name) return curr;
            curr = curr->next;
        }

        s = s->parent;
    }

    return NULL;
}

Symbol* scope_lookup_current(Scope* s, const char* name) {
    if (s == NULL) return NULL;
    
    size_t index = hash_name(name, s->capacity);
    Symbol* curr = s->buckets[index];
    
    while (curr != NULL) {
        if (curr->name == name) return curr;
        curr = curr->next;
    }

    return NULL;
}
