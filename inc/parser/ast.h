#ifndef AST_H
#define AST_H

#include <stdint.h>
#include <stddef.h>
#include "lexer.h"

typedef enum {
    AST_PROGRAM,

    AST_FUNC_DECL,

    AST_PARAM_GROUP,
    AST_BLOCK,

    AST_VAR_DECL,
    AST_SHORT_DECL,

    AST_RETURN,
    AST_ASSIGN,
    AST_CALL,
    AST_BINARY,

    AST_IDENTIFIER,
    AST_INTEGER
} ASTKind;

typedef struct AST {
    ASTKind kind;
    Token token;
    struct AST *resolved_type;
    union {
        struct {
            struct AST** targets;
            size_t target_count;
            struct AST* value;
        } assignment;

        struct {
            struct AST** stmts;
            size_t count;
            struct Scope* scope;
        } block;

        struct { 
            struct AST* name; 
            struct AST** return_types; size_t return_count; 
            struct AST** params; size_t param_count; 
            struct AST* body; 
        } func;

        struct {
            struct AST* type;
            struct AST** names;
            size_t name_count;
        } var_decl;

        struct {
            struct AST* name;
            struct AST* type;
            struct AST* value;
        } short_decl;

        struct {
            struct AST* left;
            struct AST* right;
            int op;
        } binary;

        struct {
            struct AST* callee;
            struct AST** args;
            size_t arg_count;
        } call;

        struct {
            struct AST** values;
            size_t count;
        } ret;

        int64_t int_val;
    } as;
} AST;

#endif /* AST_H */
