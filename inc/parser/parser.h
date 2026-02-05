#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stdbool.h>
#include "lexer.h"
#include "vent.h"
#include "symbol.h"
#include "ast.h"
#include "ast_buffer.h"
#include <string.h>
#include "intern.h"

typedef struct {
    TokenBuffer *tokens;
    VentContext *vent;
    ASTArena *arena;
    int pos;
    bool panic_mode;
    Scope *current_scope;
    StringInterner interner;
} Parser;

void parser_init(Parser *p, TokenBuffer *tokens, VentContext *vent, ASTArena *arena);

AST* parse_program(Parser *p);

#endif /* PARSER_H */
