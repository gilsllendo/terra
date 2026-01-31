#ifndef TOKEN_DEBUG_H
#define TOKEN_DEBUG_H

#include <stdio.h>
#include "lexer.h"
#include "token_str.h"
#include "print.h"

void lexer_debug_print_tokens(const TokenBuffer *tokens, const PrintContext *print);

#endif /* TOKEN_DEBUG_H */
