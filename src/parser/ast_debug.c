#include "ast_debug.h"
#include <stdio.h>

static void print_indent(int level) {
    for (int i = 0; i < level; i++)
        printf("  │ ");
}

static void ast_print_recursive(const AST* node, int level) {
    if (!node) return;

    print_indent(level);
    printf("%s", ast_kind_str(node->kind));

    switch (node->kind) {
        case AST_INTEGER:
            printf(": %ld\n", node->as.int_val);
            break;

        case AST_IDENTIFIER:
            printf(": %.*s\n", (int)node->token.length, node->token.start);
            break;

        case AST_FUNC_DECL:
            printf(": %.*s\n", (int)node->as.func.name->token.length, node->as.func.name->token.start);
            print_indent(level + 1);
            printf("RETURNS: ");

            for(size_t i = 0; i < node->as.func.return_count; i++) {
                printf("%.*s%s", (int)node->as.func.return_types[i]->token.length, 
                                 node->as.func.return_types[i]->token.start, 
                                 (i < node->as.func.return_count - 1) ? ", " : "");
            }

            printf("\n");

            for (size_t i = 0; i < node->as.func.param_count; i++) 
                ast_print_recursive(node->as.func.params[i], level + 1);
            ast_print_recursive(node->as.func.body, level + 1);

            break;

        case AST_PARAM_GROUP:
        case AST_VAR_DECL:
            printf(" (Type: %.*s)\n", (int)node->as.var_decl.type->token.length, node->as.var_decl.type->token.start);

            for (size_t i = 0; i < node->as.var_decl.name_count; i++) {
                print_indent(level + 1);
                printf("NAME: %.*s\n", (int)node->as.var_decl.names[i]->token.length, node->as.var_decl.names[i]->token.start);
            }

            break;

        case AST_ASSIGN:
            printf(" (Targets: %zu)\n", node->as.assignment.target_count);

            for (size_t i = 0; i < node->as.assignment.target_count; i++) {
                print_indent(level + 1);
                printf("TARGET: %.*s\n", (int)node->as.assignment.targets[i]->token.length, node->as.assignment.targets[i]->token.start);
            }

            ast_print_recursive(node->as.assignment.value, level + 1);
            break;

        case AST_RETURN:
            printf(" (Count: %zu)\n", node->as.ret.count);

            for (size_t i = 0; i < node->as.ret.count; i++) 
                ast_print_recursive(node->as.ret.values[i], level + 1);
            break;

        case AST_CALL:
            printf(": %.*s\n", (int)node->as.call.callee->token.length, node->as.call.callee->token.start);

            for (size_t i = 0; i < node->as.call.arg_count; i++) 
                ast_print_recursive(node->as.call.args[i], level + 1);
            break;

        case AST_BLOCK:
        case AST_PROGRAM:
            printf("\n");

            for (size_t i = 0; i < node->as.block.count; i++) 
                ast_print_recursive(node->as.block.stmts[i], level + 1);
            break;

        default: printf("\n"); break;
    }
}

void ast_debug_print(const AST* root, const PrintContext* print) {
    if (!print || !print->parser_debug)
        return;

    printf("=== AST Tree ===\n");

    if (!root) printf("Empty Tree\n");
    else ast_print_recursive(root, 0);

    printf("================\n\n");
}
