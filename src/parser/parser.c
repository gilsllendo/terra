#include "parser.h"
#include <string.h>
#include <stdio.h>

static AST* parse_expression(Parser* p);
static AST* parse_statement(Parser* p);
static AST* parse_block(Parser* p);
static AST* parse_function(Parser* p);
static AST* parse_var_decl(Parser* p);

void parser_init(Parser *p, TokenBuffer *tokens, VentContext *vent, ASTArena *arena) {
    p->tokens = tokens;
    p->vent = vent;
    p->arena = arena;
    p->pos = 0;
    p->panic_mode = false;

    intern_init(&p->interner, arena);
    p->current_scope = scope_new(arena, NULL);

    const char* builtins[] = {"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "bool", "void"}; // TODO: I have to implement type cheking system
    for (int i = 0; i < 10; i++) {
        const char* name = intern_string(&p->interner, arena, builtins[i], strlen(builtins[i]));
        scope_define(arena, p->current_scope, name, SYM_VAR, NULL); 
    }
}

static Token peek(Parser* p) { 
    return p->tokens->data[p->pos]; 
}

static Token previous(Parser* p) { 
    return p->tokens->data[p->pos - 1]; 
}

static bool is_at_end(Parser* p) { 
    return peek(p).kind == TOKEN_EOF; 
}

static Token advance(Parser* p) {
    if (!is_at_end(p)) p->pos++;
    return previous(p);
}

static bool check(Parser* p, TokenKind kind) {
    if (is_at_end(p)) return false;
    return peek(p).kind == kind;
}

static bool match(Parser* p, TokenKind kind) {
    if (check(p, kind)) { 
        advance(p); 
        return true; 
    }

    return false;
}

static Token consume(Parser* p, TokenKind kind, const char* message) {
    if (check(p, kind)) return advance(p);
    vent_emit(p->vent, VENT_STAGE_PARSER, VENT_SEV_ERROR, peek(p).span, message);

    p->panic_mode = true;
    return peek(p);
}

static AST* parse_primary(Parser* p) {
    if (match(p, TOKEN_INTEGER)) {
        AST* n = ast_new(p->arena, AST_INTEGER);

        n->as.int_val = previous(p).value.int_val;
        return n;
    }

    if (match(p, TOKEN_IDENTIFIER)) {
        Token id_token = previous(p);
        const char* name = intern_string(&p->interner, p->arena, id_token.start, id_token.length);
        Symbol* sym = scope_lookup(p->current_scope, name);

        if (sym == NULL) {
            char error_msg[128];
            snprintf(error_msg, sizeof(error_msg), "Undeclared identifier: '%s'", name);
            vent_emit(p->vent, VENT_STAGE_PARSER, VENT_SEV_ERROR, id_token.span, error_msg);
        }

        AST* id = ast_new(p->arena, AST_IDENTIFIER);
        id->token = id_token;

        if (match(p, TOKEN_LPAREN)) {
            AST* call = ast_new(p->arena, AST_CALL);

            call->as.call.callee = id;
            size_t cap = 4;

            call->as.call.args = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
            call->as.call.arg_count = 0;

            if (!check(p, TOKEN_RPAREN)) {
                do {
                    if (call->as.call.arg_count >= cap) {
                        cap *= 2;

                        call->as.call.args = ast_arena_realloc_array(p->arena, call->as.call.args, cap, sizeof(AST*));
                    }

                    call->as.call.args[call->as.call.arg_count++] = parse_expression(p);
                } while (match(p, TOKEN_COMMA));
            }

            consume(p, TOKEN_RPAREN, "Expected ')' after arguments.");

            return call;
        }

        return id;
    }

    if (match(p, TOKEN_LPAREN)) {
        AST* expr = parse_expression(p);

        consume(p, TOKEN_RPAREN, "Expected ')' after expression.");

        return expr;
    }

    return NULL;
}

static AST* parse_expression(Parser* p) {
    AST* left = parse_primary(p);
    if (!left) return NULL;

    while (match(p, TOKEN_PLUS) || match(p, TOKEN_MINUS)) {
        Token op = previous(p);

        AST* right = parse_primary(p);
        if (!right) break; 

        AST* node = ast_new(p->arena, AST_BINARY);

        node->as.binary.left = left;
        node->as.binary.right = right;
        node->as.binary.op = op.kind;
        left = node;
    }
    
    return left;
}

static AST* parse_var_decl(Parser *p) {
    AST* node = ast_new(p->arena, AST_VAR_DECL);

    node->as.var_decl.type = ast_new(p->arena, AST_IDENTIFIER);
    node->as.var_decl.type->token = consume(p, TOKEN_IDENTIFIER, "Expected type.");
    
    consume(p, TOKEN_COLON, "Expected ':'.");
    
    size_t cap = 4;
    node->as.var_decl.names = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
    node->as.var_decl.name_count = 0;

    do {
        Token name_tok = consume(p, TOKEN_IDENTIFIER, "Expected variable name.");
        const char* name = intern_string(&p->interner, p->arena, name_tok.start, name_tok.length);

        if (scope_lookup_current(p->current_scope, name)) {
            char msg[128];
            snprintf(msg, sizeof(msg), "Redeclaration of variable: '%s'", name);
            vent_emit(p->vent, VENT_STAGE_PARSER, VENT_SEV_ERROR, name_tok.span, msg);
        }

        scope_define(p->arena, p->current_scope, name, SYM_VAR, node);

        AST* name_ast = ast_new(p->arena, AST_IDENTIFIER);
        name_ast->token = name_tok;

        if (node->as.var_decl.name_count >= cap) {
            cap *= 2;
            node->as.var_decl.names = ast_arena_realloc_array(p->arena, node->as.var_decl.names, cap, sizeof(AST*));
        }
        node->as.var_decl.names[node->as.var_decl.name_count++] = name_ast;
    } while (match(p, TOKEN_COMMA));

    return node;
}

static AST* parse_function(Parser* p) {
    consume(p, TOKEN_FUNCTION, "Expected 'func'.");
    AST* node = ast_new(p->arena, AST_FUNC_DECL);
    
    Token name_tok = consume(p, TOKEN_IDENTIFIER, "Expected function name.");
    node->as.func.name = ast_new(p->arena, AST_IDENTIFIER);
    node->as.func.name->token = name_tok;
    
    const char* func_name = intern_string(&p->interner, p->arena, name_tok.start, name_tok.length);
    Symbol* existing = scope_lookup_current(p->current_scope, func_name);

    if (existing && existing->decl_node != NULL) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Redeclaration of function: '%s'", func_name);
        vent_emit(p->vent, VENT_STAGE_PARSER, VENT_SEV_ERROR, name_tok.span, msg);
    } else if (existing) {
        existing->decl_node = node;
    } else {
        scope_define(p->arena, p->current_scope, func_name, SYM_FUNC, node);
    }

    consume(p, TOKEN_LPAREN, "Expected '('.");
    
    Scope* outer_scope = p->current_scope;
    p->current_scope = scope_new(p->arena, outer_scope);

    size_t pcap = 4;
    node->as.func.params = ast_arena_alloc_array(p->arena, pcap, sizeof(AST*));
    node->as.func.param_count = 0;

    if (!check(p, TOKEN_RPAREN)) {
        do {
            AST* group = ast_new(p->arena, AST_PARAM_GROUP);

            group->as.var_decl.type = ast_new(p->arena, AST_IDENTIFIER);
            group->as.var_decl.type->token = consume(p, TOKEN_IDENTIFIER, "Expected type.");
            consume(p, TOKEN_COLON, "Expected ':'.");

            size_t ncap = 4;

            group->as.var_decl.names = ast_arena_alloc_array(p->arena, ncap, sizeof(AST*));
            group->as.var_decl.name_count = 0;

            while (true) {
                Token p_name_tok = consume(p, TOKEN_IDENTIFIER, "Expected param name.");

                const char* p_name = intern_string(&p->interner, p->arena, p_name_tok.start, p_name_tok.length);
                scope_define(p->arena, p->current_scope, p_name, SYM_PARAM, group);

                AST* n = ast_new(p->arena, AST_IDENTIFIER);
                n->token = p_name_tok;

                if (group->as.var_decl.name_count >= ncap) {
                    ncap *= 2;
                    group->as.var_decl.names = ast_arena_realloc_array(p->arena, group->as.var_decl.names, ncap, sizeof(AST*));
                }

                group->as.var_decl.names[group->as.var_decl.name_count++] = n;

                if (match(p, TOKEN_COMMA)) {
                    if (check(p, TOKEN_IDENTIFIER)) {
                        if (p->pos + 1 < p->tokens->length && p->tokens->data[p->pos + 1].kind == TOKEN_COLON) {
                            p->pos--; 
                            break;
                        }
                    }
                } else break;
            }

            if (node->as.func.param_count >= pcap) {
                pcap *= 2;
                node->as.func.params = ast_arena_realloc_array(p->arena, node->as.func.params, pcap, sizeof(AST*));
            }

            node->as.func.params[node->as.func.param_count++] = group;
        } while (match(p, TOKEN_COMMA));
    }

    consume(p, TOKEN_RPAREN, "Expected ')'.");
    consume(p, TOKEN_COLON, "Expected ':' before return types.");

    size_t rcap = 2;
    node->as.func.return_types = ast_arena_alloc_array(p->arena, rcap, sizeof(AST*));
    node->as.func.return_count = 0;

    if (match(p, TOKEN_LPAREN)) {
        do {
            AST* t = ast_new(p->arena, AST_IDENTIFIER);
            t->token = consume(p, TOKEN_IDENTIFIER, "Expected return type.");
            node->as.func.return_types[node->as.func.return_count++] = t;
        } while (match(p, TOKEN_COMMA));

        consume(p, TOKEN_RPAREN, "Expected ')' after return types.");
    } else {
        AST* t = ast_new(p->arena, AST_IDENTIFIER);
        t->token = consume(p, TOKEN_IDENTIFIER, "Expected return type.");
        node->as.func.return_types[node->as.func.return_count++] = t;
    }

    node->as.func.body = parse_block(p);
    p->current_scope = outer_scope;

    return node;
}

static AST* parse_statement(Parser *p) {
    if (check(p, TOKEN_FUNCTION)) return parse_function(p);

    if (match(p, TOKEN_RETURN)) {
        AST* ret = ast_new(p->arena, AST_RETURN);
        size_t vcap = 2;

        ret->as.ret.values = ast_arena_alloc_array(p->arena, vcap, sizeof(AST*));
        ret->as.ret.count = 0;

        if (!check(p, TOKEN_RBRACE)) {
            do {
                if (ret->as.ret.count >= vcap) {
                    vcap *= 2;
                    ret->as.ret.values = ast_arena_realloc_array(p->arena, ret->as.ret.values, vcap, sizeof(AST*));
                }
                ret->as.ret.values[ret->as.ret.count++] = parse_expression(p);
            } while (match(p, TOKEN_COMMA));
        }
        return ret;
    }

    if (match(p, TOKEN_VAR)) return parse_var_decl(p);

    if (check(p, TOKEN_IDENTIFIER)) {
        int look = 0;
        bool is_assign = false, is_short = false;

        while (p->pos + look < p->tokens->length) {
            TokenKind k = p->tokens->data[p->pos + look].kind;

            if (k == TOKEN_ASSIGN) { is_assign = true; break; }
            if (k == TOKEN_COLON) { is_short = true; break; }
            if (k != TOKEN_IDENTIFIER && k != TOKEN_COMMA) break;

            look++;
        }

        if (is_short) {
            AST* n = ast_new(p->arena, AST_SHORT_DECL);
            Token name_tok = advance(p);
            const char* name = intern_string(&p->interner, p->arena, name_tok.start, name_tok.length);
            
            scope_define(p->arena, p->current_scope, name, SYM_VAR, n);
            
            n->as.short_decl.name = ast_new(p->arena, AST_IDENTIFIER);
            n->as.short_decl.name->token = name_tok;
            
            consume(p, TOKEN_COLON, "Expected ':'.");
            n->as.short_decl.type = ast_new(p->arena, AST_IDENTIFIER);
            n->as.short_decl.type->token = consume(p, TOKEN_IDENTIFIER, "Expected type.");
            
            consume(p, TOKEN_ASSIGN, "Expected '='.");
            n->as.short_decl.value = parse_expression(p);
            return n;
        }

        if (is_assign) {
            AST* n = ast_new(p->arena, AST_ASSIGN);
            size_t cap = 2;
            n->as.assignment.targets = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
            n->as.assignment.target_count = 0;

            do {
                AST* target = ast_new(p->arena, AST_IDENTIFIER);
                target->token = consume(p, TOKEN_IDENTIFIER, "Expected target.");
                n->as.assignment.targets[n->as.assignment.target_count++] = target;
            } while (match(p, TOKEN_COMMA));

            consume(p, TOKEN_ASSIGN, "Expected '='.");
            n->as.assignment.value = parse_expression(p);
            return n;
        }
    }

    return parse_expression(p);
}

static AST* parse_block(Parser* p) {
    consume(p, TOKEN_LBRACE, "Expected '{'.");
    Scope* outer = p->current_scope;
    p->current_scope = scope_new(p->arena, outer);

    AST* node = ast_new(p->arena, AST_BLOCK);
    node->as.block.scope = p->current_scope;

    size_t cap = 8;
    node->as.block.stmts = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
    node->as.block.count = 0;

    while (!check(p, TOKEN_RBRACE) && !is_at_end(p)) {
        AST* stmt = parse_statement(p);

        if (stmt) {
            if (node->as.block.count >= cap) {
                cap *= 2;
                node->as.block.stmts = ast_arena_realloc_array(p->arena, node->as.block.stmts, cap, sizeof(AST*));
            }

            node->as.block.stmts[node->as.block.count++] = stmt;
        }
    }

    consume(p, TOKEN_RBRACE, "Expected '}'.");
    p->current_scope = outer;

    return node;
}

AST* parse_program(Parser* p) {
    AST* prog = ast_new(p->arena, AST_PROGRAM);
    size_t cap = 4;
    prog->as.block.stmts = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
    prog->as.block.count = 0;

    int start_pos = p->pos;

    while (!is_at_end(p)) {
        if (check(p, TOKEN_FUNCTION)) {
            advance(p);
            if (check(p, TOKEN_IDENTIFIER)) {
                Token name = advance(p);
                const char* s = intern_string(&p->interner, p->arena, name.start, name.length);
                if (!scope_lookup_current(p->current_scope, s)) {
                    scope_define(p->arena, p->current_scope, s, SYM_FUNC, NULL);
                }
                while (!is_at_end(p) && !check(p, TOKEN_LBRACE)) advance(p);
                if (check(p, TOKEN_LBRACE)) {
                    int depth = 0;
                    do {
                        if (check(p, TOKEN_LBRACE)) depth++;
                        if (check(p, TOKEN_RBRACE)) depth--;
                        advance(p);
                    } while (!is_at_end(p) && depth > 0);
                }
            }
        } else advance(p);
    }

    p->pos = start_pos;

    while (!is_at_end(p)) {
        if (check(p, TOKEN_FUNCTION)) {
            if (prog->as.block.count >= cap) {
                cap *= 2;
                prog->as.block.stmts = ast_arena_realloc_array(p->arena, prog->as.block.stmts, cap, sizeof(AST*));
            }
            prog->as.block.stmts[prog->as.block.count++] = parse_function(p);
        } else advance(p);
    }

    return prog;
}