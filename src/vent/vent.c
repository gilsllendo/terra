#include "vent.h"

void vent_context_init(VentContext *ctx) {
    memset(ctx, 0, sizeof(*ctx));
}

bool vent_emit(VentContext *ctx, VentStage stage, VentSeverity sev, VentSpan span, const char *fmt, ...) {
    if (sev == VENT_SEV_ERROR) ctx->error_count++;
    
    va_list args; va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    char *msg = malloc(len + 1);
    va_start(args, fmt); vsnprintf(msg, len + 1, fmt, args); va_end(args);

    if (ctx->count >= ctx->capacity) {
        ctx->capacity = ctx->capacity ? ctx->capacity * 2 : 8;
        ctx->diags = realloc(ctx->diags, sizeof(VentDiagnostic) * ctx->capacity);
    }

    ctx->diags[ctx->count++] = (VentDiagnostic){ stage, sev, span, msg };

    if (sev == VENT_SEV_FATAL) {
        vent_flush(ctx);
        fprintf(stderr, "FATAL ERROR: Execution halted.\n");
        exit(1);
    }

    return true;
}

void vent_flush(const VentContext *ctx) {
    for (size_t i = 0; i < ctx->count; i++) {
        VentDiagnostic *d = &ctx->diags[i];
        fprintf(stderr, "[%s] %s:%u:%u: %s\n", 
                d->severity == VENT_SEV_ERROR ? "ERROR" : "INFO",
                d->span.file, d->span.start.line, d->span.start.column, d->message);
    }
}

void vent_context_free(VentContext *ctx) {
    for (size_t i = 0; i < ctx->count; i++) free(ctx->diags[i].message);

    free(ctx->diags);
}