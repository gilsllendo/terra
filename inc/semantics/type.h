#ifndef SEM_TYPE_H
#define SEM_TYPE_H

typedef enum {
    TYPE_UNKNOWN,
    TYPE_I8, TYPE_I16, TYPE_I32, TYPE_I64,
    TYPE_U8, TYPE_U16, TYPE_U32, TYPE_U64,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_UNTYPED_INT,
    TYPE_ERROR
} ValueType;

#endif /* SEM_TYPE_H */
