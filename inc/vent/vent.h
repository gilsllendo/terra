#ifndef VENT_H
#define VENT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef enum {
    VENT_STAGE_LEXER,
    VENT_STAGE_PARSER,
} VentStage;

typedef enum {
    VENT_SEV_INFO,
    VENT_SEV_WARNING,
    VENT_SEV_ERROR,
    VENT_SEV_FATAL
} VentSeverity;

typedef struct {
    unsigned line;
    unsigned column;
} VentPos;

typedef struct {
    const char *file;
    VentPos start;
    VentPos end;
} VentSpan;

typedef struct {
    VentStage stage;
    VentSeverity severity;
    VentSpan span;
    char *message;
} VentDiagnostic;

typedef struct {
    VentDiagnostic *diags;
    size_t count;
    size_t capacity;
    unsigned error_count;
    unsigned error_limit;
} VentContext;

void vent_context_init(VentContext *ctx);
void vent_context_free(VentContext *ctx);

bool vent_emit(VentContext *ctx, VentStage stage, VentSeverity sev, VentSpan span, const char *fmt, ...);
void vent_flush(const VentContext *ctx);

#endif