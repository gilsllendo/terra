#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stdbool.h>
#include "lexer.h"
#include "vent.h"
#include "ast.h"
#include "ast_buffer.h"

typedef struct {
    TokenBuffer *tokens;
    VentContext *vent;
    ASTArena *arena;
    int pos;
    bool panic_mode;
} Parser;

void parser_init(Parser *p, TokenBuffer *tokens, VentContext *vent, ASTArena *arena);

AST* parse_program(Parser *p);

#endif /* PARSER_H */
