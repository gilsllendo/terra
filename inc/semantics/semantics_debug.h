#ifndef TERRA_SEMANTICS_DEBUG_H
#define TERRA_SEMANTICS_DEBUG_H

#include "ast.h"
#include "semantics.h"

void terra_debug_semantics(AST* node, Scope* scope, int depth);

#endif