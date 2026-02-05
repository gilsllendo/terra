#ifndef AST_BUFFER_H
#define AST_BUFFER_H

#include "ast.h"

#define ARENA_PAGE_SIZE 1024

typedef struct ASTPage {
    AST nodes[ARENA_PAGE_SIZE];
    struct ASTPage* next;
} ASTPage;

typedef struct AllocNode {
    void* ptr;
    struct AllocNode* next;
} AllocNode;

typedef struct {
    ASTPage* first;
    ASTPage* current;
    size_t index;
    AllocNode* allocs;
} ASTArena;

void ast_arena_init(ASTArena* a);
AST* ast_new(ASTArena* a, ASTKind kind);
void* ast_arena_alloc_array(ASTArena* a, size_t count, size_t size);
void* ast_arena_realloc_array(ASTArena* a, void* old_ptr, size_t new_count, size_t size);
void ast_arena_free(ASTArena* a);

#endif /* AST_BUFFER_H */
