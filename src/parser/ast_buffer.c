#include "ast_buffer.h"
#include <stdlib.h>

void ast_arena_init(ASTArena* a) {
    a->first = calloc(1, sizeof(ASTPage));

    a->current = a->first;
    a->index = 0;

    a->allocs = NULL;
}

AST* ast_new(ASTArena* a, ASTKind kind) {
    if (a->index >= ARENA_PAGE_SIZE) {
        ASTPage* new_page = calloc(1, sizeof(ASTPage));

        a->current->next = new_page;
        a->current = new_page;
        a->index = 0;
    }

    AST* node = &a->current->nodes[a->index++];
    node->kind = kind;

    return node;
}

void* ast_arena_alloc_array(ASTArena* a, size_t count, size_t size) {
    void* ptr = malloc(count * size);

    AllocNode* tracker = malloc(sizeof(AllocNode));

    tracker->ptr = ptr;
    tracker->next = a->allocs;
    a->allocs = tracker;

    return ptr;
}

void* ast_arena_realloc_array(ASTArena* a, void* old_ptr, size_t new_count, size_t size) {
    void* new_ptr = realloc(old_ptr, new_count * size);

    AllocNode* curr = a->allocs;
    while (curr) {
        if (curr->ptr == old_ptr) {
            curr->ptr = new_ptr;

            return new_ptr;
        }

        curr = curr->next;
    }

    return new_ptr;
}

void ast_arena_free(ASTArena* a) {
    AllocNode* curr_alloc = a->allocs;

    while (curr_alloc) {
        AllocNode* next = curr_alloc->next;

        free(curr_alloc->ptr);
        free(curr_alloc);

        curr_alloc = next;
    }

    ASTPage* page = a->first;
    while (page) {
        ASTPage* next = page->next;
        free(page);

        page = next;
    }
}
