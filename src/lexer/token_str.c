#include "token_str.h"

const char *token_kind_str(TokenKind kind) {
    switch (kind) {
        case TOKEN_INTEGER:    return "INTEGER";
        case TOKEN_FLOAT:      return "FLOAT";
        case TOKEN_CHAR:       return "CHAR";
        case TOKEN_STRING:     return "STRING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_FUNCTION:   return "FUNCTION";
        case TOKEN_RETURN:     return "RETURN";
        case TOKEN_PLUS:       return "PLUS";
        case TOKEN_VAR:        return "VAR";
        case TOKEN_MINUS:      return "MINUS";
        case TOKEN_MULTIPLY:   return "MULTIPLY";
        case TOKEN_DIVIDE:     return "DIVIDE";
        case TOKEN_ASSIGN:     return "ASSIGN";
        case TOKEN_LPAREN:     return "LPAREN";
        case TOKEN_RPAREN:     return "RPAREN";
        case TOKEN_LBRACE:     return "LBRACE";
        case TOKEN_RBRACE:     return "RBRACE";
        case TOKEN_COMMA:      return "COMMA";
        case TOKEN_SEMICOLON:  return "SEMICOLON";
        case TOKEN_COLON:      return "COLON";
        case TOKEN_EOF:        return "EOF";
        case TOKEN_ERROR:      return "ERROR";
        default:               return "UNKNOWN";
    }
}
