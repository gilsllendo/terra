#ifndef VENT_PRINT_H
#define VENT_PRINT_H

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    bool lexer_debug;
    bool parser_debug;
    bool semantics_debug;
} PrintContext;

#endif /* VENT_PRINT_H */
