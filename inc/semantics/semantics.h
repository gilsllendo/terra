#ifndef TERRA_SEMANTICS_H
#define TERRA_SEMANTICS_H

#include "ast.h"
#include "symbol.h"
#include "vent.h"
#include "intern.h"

void terra_analyze(AST* root, Scope* global_scope, VentContext* vent, ASTArena* arena, StringInterner* si);

#endif