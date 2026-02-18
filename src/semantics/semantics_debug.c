#include "semantics_debug.h"
#include <stdio.h>
#include <inttypes.h>

static const char* type_to_name(ValueType type) {
    switch (type) {
        case TYPE_I8: return "i8"; case TYPE_I32: return "i32";
        case TYPE_I64: return "i64"; case TYPE_U8: return "u8";
        case TYPE_BOOL: return "bool"; case TYPE_UNTYPED_INT: return "{untyped int}";
        default: return "unknown";
    }
}

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  │ ");
}

void terra_debug_semantics(AST* node, Scope* scope, int depth) {
    printf("\n=== Annotated Semantic AST ===\n");

    print_debug(node, scope, depth);

    printf("==============================\n");
}

static void print_debug(AST* node, Scope* scope, int depth) {
        if (!node) return;


    if (node->kind == AST_PROGRAM) {
        printf("PROGRAM (Analyzed)\n");
        for (size_t i = 0; i < node->as.block.count; i++)
            terra_debug_semantics(node->as.block.stmts[i], scope, depth + 1);
        return;
    }

    print_indent(depth);

    switch (node->kind) {
        case AST_FUNC_DECL: {
            printf("FUNC_DECL: %.*s\n", (int)node->as.func.name->token.length, node->as.func.name->token.start);
            print_indent(depth + 1);
            printf("└─ Returns: (");
            for (size_t i = 0; i < node->as.func.return_count; i++)
                printf("%s%s", (i > 0 ? ", " : ""), type_to_name(node->as.func.return_types[i]->resolved_type));
            printf(")\n");
            Scope* inner = node->as.func.body->as.block.scope ? node->as.func.body->as.block.scope : scope;
            terra_debug_semantics(node->as.func.body, inner, depth + 1);

            break;
        }

        case AST_IDENTIFIER: {
            char name[128]; snprintf(name, 128, "%.*s", (int)node->token.length, node->token.start);
            Symbol* s = scope_lookup(scope, name);
            ValueType display_type = (s && s->type_id != 0) ? s->type_id : node->resolved_type;
            printf("IDENTIFIER: %s [Type: %s]\n", name, type_to_name(display_type));
  
            break;
        }

        case AST_BINARY:
            printf("BINARY: %.*s [Result: %s]\n", (int)node->token.length, node->token.start, type_to_name(node->resolved_type));
            terra_debug_semantics(node->as.binary.left, scope, depth + 1);
            terra_debug_semantics(node->as.binary.right, scope, depth + 1);

            break;

        case AST_CALL: {
            char buf[256]; snprintf(buf, 256, "%.*s", (int)node->as.call.callee->token.length, node->as.call.callee->token.start);
            Symbol* f = scope_lookup(scope, buf);

            printf("CALL: %s", buf);

            if (f && f->decl_node) {
                printf(" [Returns: (");
                for (size_t i = 0; i < f->decl_node->as.func.return_count; i++)
                    printf("%s%s", (i > 0 ? ", " : ""), type_to_name(f->decl_node->as.func.return_types[i]->resolved_type));
                printf(")]\n");
            } else printf(" [Type: %s]\n", type_to_name(node->resolved_type));
            for (size_t i = 0; i < node->as.call.arg_count; i++) terra_debug_semantics(node->as.call.args[i], scope, depth + 1);

            break;
        }

        case AST_VAR_DECL:
            printf("VAR_DECL [Type: %s]\n", type_to_name(node->as.var_decl.type->resolved_type));
            for (size_t i = 0; i < node->as.var_decl.name_count; i++)
                terra_debug_semantics(node->as.var_decl.names[i], scope, depth + 1);
                
            break;

        case AST_BLOCK: {
            printf("BLOCK [Scope: %p]\n", (void*)node->as.block.scope);
            Scope* s = node->as.block.scope ? node->as.block.scope : scope;
            for (size_t i = 0; i < node->as.block.count; i++) terra_debug_semantics(node->as.block.stmts[i], s, depth + 1);

            break;
        }

        case AST_INTEGER: printf("INTEGER: %" PRId64 " [Type: %s]\n", node->as.int_val, type_to_name(node->resolved_type)); break;
        case AST_ASSIGN:
            printf("ASSIGN [Targets: %zu]\n", node->as.assignment.target_count);
            for (size_t i = 0; i < node->as.assignment.target_count; i++) terra_debug_semantics(node->as.assignment.targets[i], scope, depth + 1);
            terra_debug_semantics(node->as.assignment.value, scope, depth + 1);

            break;
        case AST_RETURN:
            printf("RETURN [Values: %zu]\n", node->as.ret.count);
            for (size_t i = 0; i < node->as.ret.count; i++) terra_debug_semantics(node->as.ret.values[i], scope, depth + 1);
            
            break;
        default: break;
    }
}
