#include "parser/parser.h"
#include <string.h>

static AST* parse_expression(Parser* p);
static AST* parse_statement(Parser* p);
static AST* parse_block(Parser* p);

void parser_init(Parser *p, TokenBuffer *tokens, VentContext *vent, ASTArena *arena) {
    p->tokens = tokens;
    p->vent = vent;
    p->arena = arena;
    p->pos = 0;
    p->panic_mode = false;
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
    if (check(p, kind)) { advance(p); return true; }
    return false;
}

static Token consume(Parser* p, TokenKind kind, const char* message) {
    if (check(p, kind))
        return advance(p);

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
        AST* id = ast_new(p->arena, AST_IDENTIFIER);
        id->token = previous(p);
        
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
        AST* name = ast_new(p->arena, AST_IDENTIFIER);
        name->token = consume(p, TOKEN_IDENTIFIER, "Expected name.");

        if (node->as.var_decl.name_count >= cap) {
            cap *= 2;
            node->as.var_decl.names = ast_arena_realloc_array(p->arena, node->as.var_decl.names, cap, sizeof(AST*));
        }

        node->as.var_decl.names[node->as.var_decl.name_count++] = name;
    } while (match(p, TOKEN_COMMA));

    return node;
}

static AST* parse_function(Parser* p) {
    AST* node = ast_new(p->arena, AST_FUNC_DECL);

    node->as.func.name = ast_new(p->arena, AST_IDENTIFIER);
    node->as.func.name->token = consume(p, TOKEN_IDENTIFIER, "Expected function name.");
    
    consume(p, TOKEN_LPAREN, "Expected '('.");

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
                AST* n = ast_new(p->arena, AST_IDENTIFIER);

                n->token = consume(p, TOKEN_IDENTIFIER, "Expected name.");

                if (group->as.var_decl.name_count >= ncap) {
                    ncap *= 2;
                    group->as.var_decl.names = ast_arena_realloc_array(p->arena, group->as.var_decl.names, ncap, sizeof(AST*));
                }

                group->as.var_decl.names[group->as.var_decl.name_count++] = n;

                if (check(p, TOKEN_COMMA)) {
                    if (p->pos + 2 < p->tokens->length && p->tokens->data[p->pos + 2].kind == TOKEN_COLON) break;
                    advance(p);
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
            if (node->as.func.return_count >= rcap) {
                rcap *= 2;
                node->as.func.return_types = ast_arena_realloc_array(p->arena, node->as.func.return_types, rcap, sizeof(AST*));
            }

            AST* type_node = ast_new(p->arena, AST_IDENTIFIER);
            type_node->token = consume(p, TOKEN_IDENTIFIER, "Expected return type.");
            node->as.func.return_types[node->as.func.return_count++] = type_node;

        } while (match(p, TOKEN_COMMA));

        consume(p, TOKEN_RPAREN, "Expected ')' after return types.");
    } else {
        AST* type_node = ast_new(p->arena, AST_IDENTIFIER);

        type_node->token = consume(p, TOKEN_IDENTIFIER, "Expected return type.");
        node->as.func.return_types[node->as.func.return_count++] = type_node;
    }
    
    node->as.func.body = parse_block(p);

    return node;
}

static AST* parse_statement(Parser *p) {
    if (match(p, TOKEN_RETURN)) {
        AST* ret_node = ast_new(p->arena, AST_RETURN);
        size_t vcap = 2;

        ret_node->as.ret.values = ast_arena_alloc_array(p->arena, vcap, sizeof(AST*));
        ret_node->as.ret.count = 0;

        do {
            if (ret_node->as.ret.count >= vcap) {
                vcap *= 2;
                ret_node->as.ret.values = ast_arena_realloc_array(p->arena, ret_node->as.ret.values, vcap, sizeof(AST*));
            }

            ret_node->as.ret.values[ret_node->as.ret.count++] = parse_expression(p);
        } while (match(p, TOKEN_COMMA));

        match(p, TOKEN_SEMICOLON);

        return ret_node;
    }

    if (match(p, TOKEN_VAR)) {
        AST* v = parse_var_decl(p);

        match(p, TOKEN_SEMICOLON);

        return v;
    }

    if (check(p, TOKEN_IDENTIFIER)) {
        int look = 0;
        bool is_assign = false, is_short = false;

        while (p->pos + look < p->tokens->length) {
            TokenKind k = p->tokens->data[p->pos + look].kind;
            
            if (k == TOKEN_ASSIGN) { is_assign = true; break; }
            if (k == TOKEN_COLON)  { is_short = true; break; }
            if (k != TOKEN_IDENTIFIER && k != TOKEN_COMMA) break;

            look++;
        }

        if (is_short) {
            AST* node = ast_new(p->arena, AST_SHORT_DECL);

            node->as.short_decl.name = ast_new(p->arena, AST_IDENTIFIER);
            node->as.short_decl.name->token = advance(p);

            consume(p, TOKEN_COLON, "Expected ':'.");
            node->as.short_decl.type = ast_new(p->arena, AST_IDENTIFIER);
            node->as.short_decl.type->token = consume(p, TOKEN_IDENTIFIER, "Expected type.");

            consume(p, TOKEN_ASSIGN, "Expected '='.");
            node->as.short_decl.value = parse_expression(p);

            match(p, TOKEN_SEMICOLON);

            return node;
        }

        if (is_assign) {
            AST* node = ast_new(p->arena, AST_ASSIGN);
            size_t cap = 2;

            node->as.assignment.targets = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
            node->as.assignment.target_count = 0;

            do {
                if (node->as.assignment.target_count >= cap) {
                    cap *= 2;
                    node->as.assignment.targets = ast_arena_realloc_array(p->arena, node->as.assignment.targets, cap, sizeof(AST*));
                }

                AST* target = ast_new(p->arena, AST_IDENTIFIER);
                target->token = consume(p, TOKEN_IDENTIFIER, "Expected target.");
                node->as.assignment.targets[node->as.assignment.target_count++] = target;
            } while (match(p, TOKEN_COMMA));

            consume(p, TOKEN_ASSIGN, "Expected '='.");
            node->as.assignment.value = parse_expression(p);
            match(p, TOKEN_SEMICOLON);

            return node;
        }
    }

    AST* expr = parse_expression(p);
    match(p, TOKEN_SEMICOLON);

    return expr;
}

static AST* parse_block(Parser* p) {
    consume(p, TOKEN_LBRACE, "Expected '{'.");
    AST* node = ast_new(p->arena, AST_BLOCK);
    size_t cap = 8;

    node->as.block.stmts = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
    node->as.block.count = 0;

    while (!check(p, TOKEN_RBRACE) && !is_at_end(p)) {
        unsigned start = p->pos;
        AST* stmt = parse_statement(p);

        if (stmt) {
            if (node->as.block.count >= cap) {
                cap *= 2;
                node->as.block.stmts = ast_arena_realloc_array(p->arena, node->as.block.stmts, cap, sizeof(AST*));
            }

            node->as.block.stmts[node->as.block.count++] = stmt;
        }

        if (p->pos == start) advance(p); 
        if (p->panic_mode) {
            p->panic_mode = false;
            while (!is_at_end(p) && !check(p, TOKEN_SEMICOLON) && !check(p, TOKEN_RBRACE)) advance(p);
            match(p, TOKEN_SEMICOLON);
        }
    }

    consume(p, TOKEN_RBRACE, "Expected '}'.");

    return node;
}

AST* parse_program(Parser* p) {
    AST* prog = ast_new(p->arena, AST_PROGRAM);
    size_t cap = 4;

    prog->as.block.stmts = ast_arena_alloc_array(p->arena, cap, sizeof(AST*));
    prog->as.block.count = 0;

    while (!is_at_end(p)) {
        if (match(p, TOKEN_FUNCTION)) {
            if (prog->as.block.count >= cap) {
                cap *= 2;
                prog->as.block.stmts = ast_arena_realloc_array(p->arena, prog->as.block.stmts, cap, sizeof(AST*));
            }

            prog->as.block.stmts[prog->as.block.count++] = parse_function(p);
        } else advance(p);
    }
    
    return prog;
}
