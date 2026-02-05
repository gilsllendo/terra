#ifndef SYMBOL_DEBUG_H
#define SYMBOL_DEBUG_H

#include <stdio.h>
#include "symbol.h"
#include "print.h"
#include "symbol_str.h"

void semantics_debug_print_tree(const Scope *global_scope, AST *root, const PrintContext *print);

#endif /* SYMBOL_DEBUG_H */
