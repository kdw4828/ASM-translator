#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "symbol_table.h"

typedef struct symtab_entry {
    char* name;
    char* type;
    char* info;
    int scope;
    int num;
} symtab_entry;

typedef enum {
    INTEGER_CONST,
    SYMBOL_PTR,
    PARAM,
    CALL,
    RETRIEVE,
    ASSG_STMT,
    ENTER,
    LEAVE,
    RETURN_STMT,
    UNUSED,
    LABEL,
    IF_EQ,
    IF_NE,
    IF_GT,
    IF_GE,
    IF_LT,
    IF_LE,
    GOTO,
    PLUS,
    MINUS,
    MULTIPLE,
    DIVIDE,
    UNARY
} OpType;

typedef struct {
    OpType operand_type;
    union {
        int iconst;
        symtab_entry* stptr;
        char* name;
    } val;
} Operand;

typedef struct quad {
    OpType op;
    Operand src1;
    Operand src2;
    Operand dest;
    struct quad* next;
} Quad;


#endif