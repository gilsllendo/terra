#ifndef LEXER_H
#define LEXER_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vent.h"

typedef enum {
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_CHAR,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_ASSIGN,
    TOKEN_VAR,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_EOF,
    TOKEN_ERROR,
} TokenKind;

typedef struct {
    TokenKind kind;
    const char *start;
    unsigned length;
    VentSpan span;
    union {
        long int_val;
        double float_val;
        char char_val;
    } value;
} Token;

typedef struct {
    Token *data;
    unsigned length;
    unsigned capacity;
} TokenBuffer;

typedef struct {
    const char *src;
    const char *file;
    unsigned pos;
    unsigned line;
    unsigned column;
    TokenBuffer *tokens;
    VentContext *vent;
} Lexer;

void token_buffer_init(TokenBuffer *buf, VentContext *vent);
void token_buffer_push(TokenBuffer *buf, VentContext *vent, Token tok);
void token_buffer_free(TokenBuffer *buf);

void lexer_init(Lexer *l, const char *src, const char *file, TokenBuffer *out, VentContext *v);
void lexer_run(Lexer *l);

#endif