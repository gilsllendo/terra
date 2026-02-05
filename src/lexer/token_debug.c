#include "token_debug.h"

void lexer_debug_print_tokens(const TokenBuffer *tokens, const PrintContext *print) {
    if (!print || !print->lexer_debug) return;

    printf("=== Lexer tokens ===\n");
    
    if (!tokens || !tokens->data) {
        printf("Error: Token buffer is empty or uninitialized.\n");
        return;
    }

    for (unsigned i = 0; i < tokens->length; ++i) {
        const Token *t = &tokens->data[i];

        printf("%-12s %u:%u  '%.*s'\n",
               token_kind_str(t->kind),
               t->span.start.line,
               t->span.start.column,
               (int)t->length,
               t->start ? t->start : "<null>");
    }
    
    printf("Total tokens: %u\n\n", tokens->length);
}
