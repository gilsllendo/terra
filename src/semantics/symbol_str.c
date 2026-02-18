#include "symbol_str.h"

const char* symbol_kind_str(SymbolKind kind) {
    switch (kind) {
        case SYM_VAR:   return "VARIABLE";
        case SYM_FUNC:  return "FUNCTION";
        case SYM_PARAM: return "PARAMETER";
        case SYM_TYPE:  return "TYPE";
        default:        return "UNKNOWN";
    }
}
