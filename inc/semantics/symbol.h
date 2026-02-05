#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"
#include "ast_buffer.h"
#include <string.h>

typedef enum {
    SYM_VAR,
    SYM_FUNC,
    SYM_TYPE,
    SYM_PARAM
} SymbolKind;

typedef struct Symbol {
    const char* name;
    SymbolKind kind;
    AST* decl_node;
    struct Symbol* next;
} Symbol;

typedef struct Scope {
    Symbol** buckets;
    size_t capacity;
    struct Scope* parent; 
} Scope;

Scope* scope_new(ASTArena* arena, Scope* parent);
void scope_define(ASTArena* arena, Scope* s, const char* name, SymbolKind kind, AST* node);
Symbol* scope_lookup(Scope* s, const char* name);
Symbol* scope_lookup_current(Scope* s, const char* name);

#endif /* SYMBOL_H */
