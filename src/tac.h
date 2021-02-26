#ifndef TAC_H
#define TAC_H

#include "symbol_table.h"

typedef enum {
    TAC_UNDEF = 1, TAC_ADD, TAC_SUB, TAC_MULT, TAC_DIV, TAC_POS, TAC_NEG, TAC_CPY, TAC_GOTO, TAC_IFZ, TAC_IFNZ, TAC_MOD, TAC_AND, TAC_OR, TAC_NOT,
    TAC_LT, TAC_GT, TAC_NEQ, TAC_LTE, TAC_GTE, TAC_EQ, TAC_VAR, TAC_LABEL, TAC_PRINT, TAC_BEGINFUNC, TAC_ENDFUNC, TAC_ARGLIST, TAC_BEGINPROC,
    TAC_ENDPROC, TAC_BEGINPROG, TAC_ENDPROG
} TacOp;

typedef struct _Tac {
    struct _Tac *prev;
    struct _Tac *next;
    TacOp op;

    union {
        Symbol *symb;
        struct _Tac *label;
    } a;

    union {
        Symbol *symb;
        struct _Tac *label;
    } b;

    union {
        Symbol *symb;
        struct _Tac *label;
    } c;
} Tac;

typedef struct _ExprNode {
    struct _ExprNode *next;
    Tac *tac;
    Symbol *result;

    Symbol *array;
    Symbol *offset;
    int dim;
} ExprNode;

Tac* make_tac(TacOp, Symbol *, Symbol *, Symbol *);
Tac* join_tac(Tac *, Tac *);
Tac* tac_program(Symbol *, Symbol *, Tac *);
Tac* tac_declare(Symbol *);

Tac *program_tac;

#endif
