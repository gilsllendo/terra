#include "symbol_debug.h"

static void print_single_scope_level(const Scope *s, const char* label, Token span_tok) {
    if (!s) return;
    
    if (span_tok.start != NULL) {
        printf("--- Scope: %s [%p] (Line %u:%u) ---\n", 
               label, (void*)s, span_tok.span.start.line, span_tok.span.start.column);
    } else {
        printf("--- Scope: %s [%p] ---\n", label, (void*)s);
    }
    
    int count = 0;
    for (size_t i = 0; i < s->capacity; i++) {
        Symbol *sym = s->buckets[i];
        while (sym) {
            printf("  %-12s  %-16s  node:%p\n", 
                   symbol_kind_str(sym->kind),
                   sym->name, 
                   (void*)sym->decl_node);
            count++;
            sym = sym->next;
        }
    }
    
    if (count == 0) printf("  <empty scope>\n");
    else printf("  (Total symbols: %d)\n", count);
    printf("\n");
}

static void semantics_walk_and_print(AST* node, const PrintContext* print) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->as.block.count; i++) {
                semantics_walk_and_print(node->as.block.stmts[i], print);
            }
            break;

        case AST_FUNC_DECL: {
            const char* func_name = node->as.func.name->token.start;
            int func_len = (int)node->as.func.name->token.length;
            
            char label[256];
            snprintf(label, sizeof(label), "Function '%.*s'", func_len, func_name);
            
            if (node->as.func.body && node->as.func.body->kind == AST_BLOCK) {
                print_single_scope_level(node->as.func.body->as.block.scope, label, node->as.func.name->token);
            }
            
            semantics_walk_and_print(node->as.func.body, print);
            break;
        }

        case AST_BLOCK:
            for (size_t i = 0; i < node->as.block.count; i++) {
                semantics_walk_and_print(node->as.block.stmts[i], print);
            }
            break;

        default: break;
    }
}

void semantics_debug_print_tree(const Scope *global_scope, AST *root, const PrintContext *print) {
    if (!print || !print->semantics_debug) return;

    printf("\n=== Semantics Debug: Scope Tree ===\n");
    
    print_single_scope_level(global_scope, "Global", root->token);
    
    semantics_walk_and_print(root, print);
    
    printf("==================================\n\n");
}