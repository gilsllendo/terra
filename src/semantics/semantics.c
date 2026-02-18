#include "semantics.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    VentContext * vent;
    Scope * current_scope;
    ASTArena * arena;
    StringInterner * si;
}
Analyzer;

static void analyze_node(Analyzer * a, AST * node);

static Symbol * lookup_symbol(Analyzer * a, Token tok) {
    if (!tok.start || tok.length == 0) return NULL;
    const char * name = intern_string(a -> si, a -> arena, tok.start, tok.length);
    Symbol * s = scope_lookup(a -> current_scope, name);

    return s;
}
static bool types_match(ValueType expected, ValueType actual) {
    if (actual == TYPE_ERROR || expected == TYPE_ERROR) return true;
    if (expected == actual) return true;
    if (actual == TYPE_UNTYPED_INT) return (expected >= TYPE_I8 && expected <= TYPE_U64);

    return false;
}

static
const char * type_to_name(ValueType type) {
    switch (type) {
    case TYPE_I8:
        return "i8";
    case TYPE_I32:
        return "i32";
    case TYPE_I64:
        return "i64";
    case TYPE_U8:
        return "u8";
    case TYPE_BOOL:
        return "bool";
    case TYPE_UNTYPED_INT:
        return "{untyped int}";
    default:
        return "unknown";
    }
}

static void resolve_headers_recursive(Analyzer * a, AST * node) {
    if (!node) return;

    if (node -> kind == AST_FUNC_DECL) {
        Token name_tok = node -> as.func.name -> token;
        const char * f_name = intern_string(a -> si, a -> arena, name_tok.start, name_tok.length);
        printf("[Pass 1] Resolving Function: %s\n", f_name);

        Symbol * f_sym = scope_lookup(a -> current_scope, f_name);
        if (f_sym) f_sym -> decl_node = node;

        for (size_t i = 0; i < node -> as.func.return_count; i++) {
            AST * ret_type_node = node -> as.func.return_types[i];
            Symbol * r_sym = lookup_symbol(a, ret_type_node -> token);
            if (r_sym) {
                ret_type_node -> resolved_type = r_sym -> type_id;
                if (i == 0 && f_sym) f_sym -> type_id = r_sym -> type_id;
            }
        }

        if (node -> as.func.body && node -> as.func.body -> as.block.scope) {
            Scope * f_scope = node -> as.func.body -> as.block.scope;

            for (size_t i = 0; i < node -> as.func.param_count; i++) {
                AST * group = node -> as.func.params[i];

                Symbol * t_sym = lookup_symbol(a, group -> as.var_decl.type -> token);
                ValueType tid = t_sym ? t_sym -> type_id : TYPE_ERROR;
                group -> as.var_decl.type -> resolved_type = tid;

                for (size_t j = 0; j < group -> as.var_decl.name_count; j++) {
                    AST * p_node = group -> as.var_decl.names[j];
                    const char * p_str = intern_string(a -> si, a -> arena, p_node -> token.start, p_node -> token.length);

                    Symbol * p_sym = scope_lookup_current(f_scope, p_str);
                    if (p_sym) {
                        p_sym -> type_id = tid;
                        p_node -> resolved_type = tid;
                        printf("  -> Parameter '%s' matched in scope and set to type %s\n", p_str, type_to_name(tid));
                    } else {
                        printf("  !! ERROR: Parameter '%s' NOT FOUND in function scope %p\n", p_str, (void * ) f_scope);
                    }
                }
            }

            Scope * prev = a -> current_scope;
            a -> current_scope = f_scope;
            resolve_headers_recursive(a, node -> as.func.body);
            a -> current_scope = prev;
        }
    } else if (node -> kind == AST_BLOCK || node -> kind == AST_PROGRAM) {
        for (size_t i = 0; i < node -> as.block.count; i++)
            resolve_headers_recursive(a, node -> as.block.stmts[i]);
    }
}

static void analyze_node(Analyzer * a, AST * node) {
    if (!node) return;

    switch (node -> kind) {
    case AST_PROGRAM:
    case AST_BLOCK: {
        Scope * prev = a -> current_scope;
        if (node -> as.block.scope) a -> current_scope = node -> as.block.scope;
        for (size_t i = 0; i < node -> as.block.count; i++) analyze_node(a, node -> as.block.stmts[i]);
        a -> current_scope = prev;

        break;
    }

    case AST_FUNC_DECL: {
        if (node -> as.func.body && node -> as.func.body -> as.block.scope) {
            Scope * prev = a -> current_scope;
            a -> current_scope = node -> as.func.body -> as.block.scope;
            analyze_node(a, node -> as.func.body);
            a -> current_scope = prev;
        }

        break;
    }

    case AST_VAR_DECL: {
        Symbol * t_sym = lookup_symbol(a, node -> as.var_decl.type -> token);
        ValueType tid = t_sym ? t_sym -> type_id : TYPE_ERROR;
        node -> as.var_decl.type -> resolved_type = tid;

        for (size_t i = 0; i < node -> as.var_decl.name_count; i++) {
            AST * name_node = node -> as.var_decl.names[i];
            Symbol * v_sym = lookup_symbol(a, name_node -> token);
            if (v_sym) v_sym -> type_id = tid;
            name_node -> resolved_type = tid;
        }
        break;
    }

    case AST_SHORT_DECL: {
        analyze_node(a, node -> as.short_decl.value);
        ValueType val_type = node -> as.short_decl.value -> resolved_type;

        Symbol * t_sym = lookup_symbol(a, node -> as.short_decl.type -> token);
        ValueType final_type = t_sym ? t_sym -> type_id : val_type;

        node -> as.short_decl.type -> resolved_type = final_type;
        Symbol * v_sym = lookup_symbol(a, node -> as.short_decl.name -> token);
        if (v_sym) v_sym -> type_id = final_type;
        node -> as.short_decl.name -> resolved_type = final_type;

        break;
    }

    case AST_ASSIGN: {
        analyze_node(a, node -> as.assignment.value);
        AST * rhs = node -> as.assignment.value;

        if (rhs && rhs -> kind == AST_CALL) {
            Symbol * f_sym = lookup_symbol(a, rhs -> as.call.callee -> token);
            if (f_sym && f_sym -> decl_node) {
                AST * decl = f_sym -> decl_node;
                for (size_t i = 0; i < node -> as.assignment.target_count; i++) {
                    if (i >= decl -> as.func.return_count) break;

                    AST * target = node -> as.assignment.targets[i];
                    ValueType rt = decl -> as.func.return_types[i] -> resolved_type;

                    Symbol * ts = lookup_symbol(a, target -> token);
                    if (ts) ts -> type_id = rt;

                    target -> resolved_type = rt;
                }
            }
        } else {
            for (size_t i = 0; i < node -> as.assignment.target_count; i++) {
                AST * target = node -> as.assignment.targets[i];
                Symbol * ts = lookup_symbol(a, target -> token);

                if (ts) {
                    if (ts -> type_id == TYPE_ERROR) ts -> type_id = rhs -> resolved_type;
                    target -> resolved_type = ts -> type_id;
                }
            }
        }

        break;
    }

    case AST_BINARY: {
        analyze_node(a, node -> as.binary.left);
        analyze_node(a, node -> as.binary.right);
        ValueType lt = node -> as.binary.left -> resolved_type;
        ValueType rt = node -> as.binary.right -> resolved_type;

        if (!types_match(lt, rt) && !types_match(rt, lt)) {
            vent_emit(a -> vent, VENT_STAGE_SEMANTICS, VENT_SEV_ERROR, node -> token.span, "Type Mismatch");
        }
        node -> resolved_type = (lt == TYPE_UNTYPED_INT) ? rt : lt;

        break;
    }

    case AST_IDENTIFIER: {
        Symbol * sym = lookup_symbol(a, node -> token);
        node -> resolved_type = sym ? sym -> type_id : TYPE_ERROR;
        break;
    }

    case AST_INTEGER:
        node -> resolved_type = TYPE_UNTYPED_INT;
        break;
    case AST_CALL: {
        for (size_t i = 0; i < node -> as.call.arg_count; i++)
            analyze_node(a, node -> as.call.args[i]);

        Symbol * s = lookup_symbol(a, node -> as.call.callee -> token);
        if (s && s -> decl_node && s -> decl_node -> as.func.return_count > 0) {
            node -> resolved_type = s -> decl_node -> as.func.return_types[0] -> resolved_type;
        } else {
            node -> resolved_type = s ? s -> type_id : TYPE_ERROR;
        }
        node -> as.call.callee -> resolved_type = node -> resolved_type;

        break;
    }
    case AST_RETURN: {
        for (size_t i = 0; i < node -> as.ret.count; i++) analyze_node(a, node -> as.ret.values[i]);

        break;
    }
    default:
        break;
    }
}

void terra_analyze(AST * root, Scope * global, VentContext * vent, ASTArena * arena, StringInterner * si) {
    Analyzer a = {
        vent,
        global,
        arena,
        si
    };

    resolve_headers_recursive( & a, root);
    analyze_node( & a, root);
}
