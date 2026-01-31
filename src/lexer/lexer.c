#include "lexer.h"

void token_buffer_init(TokenBuffer *buf, VentContext *vent) {
    buf->length = 0;
    buf->capacity = 64;
    buf->data = malloc(sizeof(Token) * buf->capacity);

    if (!buf->data) {
        vent_emit(
            vent,
            VENT_STAGE_LEXER,
            VENT_SEV_FATAL,
            (VentSpan){0},
            "failed to allocate initial token buffer"
        );
    }
}

void token_buffer_free(TokenBuffer *buf) {
    if (buf->data) free(buf->data);

    buf->data = NULL;
    buf->length = 0;
    buf->capacity = 0;
}

void token_buffer_push(TokenBuffer *buf, VentContext *vent, Token tok) {
    if (buf->length >= buf->capacity) {
        unsigned new_cap = buf->capacity * 2;
        Token *new_data = realloc(buf->data, sizeof(Token) * new_cap);

        if (!new_data) {
            vent_emit(
                vent,
                VENT_STAGE_LEXER,
                VENT_SEV_FATAL,
                tok.span, 
                "out of memory while expanding token buffer"
            );

            return; 
        }

        buf->data = new_data;
        buf->capacity = new_cap;
    }

    buf->data[buf->length++] = tok;
}

static char cur(Lexer *l) {
    return l->src[l->pos];
}

static char peek(Lexer *l) {
    if (cur(l) == '\0') return '\0';

    return l->src[l->pos + 1];
}

static void adv(Lexer *l) {
    if (cur(l) == '\n') {
        l->line++;
        l->column = 1;
    } else {
        l->column++;
    }

    l->pos++;
}

static bool match(Lexer *l, char expected) {
    if (cur(l) != expected) return false;

    adv(l);

    return true;
}

static TokenKind keyword_kind(const char *s, unsigned len) {
    if (len == 4 && strncmp(s, "func", 4) == 0)   return TOKEN_FUNCTION;
    if (len == 6 && strncmp(s, "return", 6) == 0) return TOKEN_RETURN;
    if (len == 2 && strncmp(s, "if", 2) == 0)     return TOKEN_IF;
    if (len == 4 && strncmp(s, "else", 4) == 0)   return TOKEN_ELSE;
    return TOKEN_IDENTIFIER;
}

void lexer_init(Lexer *l, const char *source, const char *file, 
                TokenBuffer *out_tokens, VentContext *vent) {
    l->src    = source;
    l->file   = file;
    l->pos    = 0;
    l->line   = 1;
    l->column = 1;
    l->tokens = out_tokens;
    l->vent   = vent;
}

void lexer_run(Lexer *l) {
    for (;;) {
        char c = cur(l);
        if (isspace((unsigned char)c)) {
            adv(l);
            continue;
        }

        if (c == '/' && peek(l) == '/') {
            while (cur(l) != '\n' && cur(l) != '\0') adv(l);

            continue;
        }

        VentPos start_pos = { l->line, l->column };
        
        Token tok = {0};
        tok.start      = l->src + l->pos;
        tok.span.file  = l->file;
        tok.span.start = start_pos;

        if (c == '\0') {
            tok.kind     = TOKEN_EOF;
            tok.length   = 0;
            tok.span.end = start_pos;

            token_buffer_push(l->tokens, l->vent, tok);

            return;
        }

        if (isdigit((unsigned char)c)) {
            bool is_float = false;
            while (isdigit((unsigned char)cur(l))) adv(l);

            if (cur(l) == '.' && isdigit((unsigned char)peek(l))) {
                is_float = true;
                
                adv(l);

                while (isdigit((unsigned char)cur(l))) adv(l);
            }

            tok.length = (unsigned)(l->src + l->pos - tok.start);
            tok.kind   = is_float ? TOKEN_FLOAT : TOKEN_INTEGER;

            if (tok.length < 127) {
                char tmp[128];
                memcpy(tmp, tok.start, tok.length);
                tmp[tok.length] = '\0';
                if (is_float) tok.value.float_val = strtod(tmp, NULL);
                else          tok.value.int_val   = strtol(tmp, NULL, 10);
            } else {
                vent_emit(
                    l->vent,
                    VENT_STAGE_LEXER,
                    VENT_SEV_ERROR,
                    tok.span, 
                    "numeric literal exceeds maximum buffer length"
                );
            }

            tok.span.end = (VentPos){ l->line, l->column };
            token_buffer_push(l->tokens, l->vent, tok);

            continue;
        }

        if (isalpha((unsigned char)c) || c == '_') {
            while (isalnum((unsigned char)cur(l)) || cur(l) == '_') adv(l);
            
            tok.length = (unsigned)(l->src + l->pos - tok.start);
            tok.kind   = keyword_kind(tok.start, tok.length);
            
            tok.span.end = (VentPos){ l->line, l->column };
            token_buffer_push(l->tokens, l->vent, tok);

            continue;
        }

        adv(l);
        switch (c) {
            case '+': tok.kind = TOKEN_PLUS; break;
            case '-': tok.kind = TOKEN_MINUS; break;
            case '*': tok.kind = TOKEN_MULTIPLY; break;
            case '/': tok.kind = TOKEN_DIVIDE; break;
            case '(': tok.kind = TOKEN_LPAREN; break;
            case ')': tok.kind = TOKEN_RPAREN; break;
            case '{': tok.kind = TOKEN_LBRACE; break;
            case '}': tok.kind = TOKEN_RBRACE; break;
            case ',': tok.kind = TOKEN_COMMA; break;
            case ';': tok.kind = TOKEN_SEMICOLON; break;

            case '=': 
                tok.kind = match(l, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_ASSIGN; 
                break;
            case '!': 
                if (match(l, '=')) {
                    tok.kind = TOKEN_BANG_EQUAL;
                } else {
                    vent_emit(l->vent, VENT_STAGE_LEXER, VENT_SEV_ERROR, tok.span, 
                              "unexpected character '!' (did you mean '!=')?");
                    tok.kind = TOKEN_ERROR;
                }
                break;

            default:
                vent_emit(l->vent, VENT_STAGE_LEXER, VENT_SEV_ERROR, tok.span, 
                          "unexpected character '%c'", c);
                tok.kind = TOKEN_ERROR;
                break;
        }

        tok.length   = (unsigned)(l->src + l->pos - tok.start);
        tok.span.end = (VentPos){ l->line, l->column };

        token_buffer_push(l->tokens, l->vent, tok);
    }
}