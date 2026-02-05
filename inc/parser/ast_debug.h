#ifndef AST_DEBUG_H
#define AST_DEBUG_H

#include "ast.h"
#include "print.h"
#include "ast_str.h"
#include <stdio.h>

void ast_debug_print(const AST* root, const PrintContext* print);

#endif /* AST_DEBUG_H */
