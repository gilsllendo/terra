#include "ast_str.h"

const char* ast_kind_str(ASTKind kind) {
    switch (kind) {
        case AST_PROGRAM:    return "PROGRAM";
        case AST_FUNC_DECL:  return "FUNC_DECL";
        case AST_PARAM_GROUP:return "PARAM_GROUP";
        case AST_BLOCK:      return "BLOCK";
        case AST_VAR_DECL:   return "VAR_DECL";
        case AST_SHORT_DECL: return "SHORT_DECL";
        case AST_RETURN:     return "RETURN";
        case AST_ASSIGN:     return "ASSIGN";
        case AST_CALL:       return "CALL";
        case AST_BINARY:     return "BINARY";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_INTEGER:    return "INTEGER";
        default:             return "UNKNOWN";
    }
}
