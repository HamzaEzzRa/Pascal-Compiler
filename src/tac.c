#include <stdio.h>
#include <scanner.h>
#include "tac.h"

Tac* make_tac(TacOp op, Symbol *a, Symbol *b, Symbol *c) {
    Tac *new_tac = malloc(sizeof(Tac));
    new_tac->prev = NULL;
    new_tac->next = NULL;
    
    new_tac->op = op;
    new_tac->a.symb = a;
    new_tac->b.symb = b;
    new_tac->c.symb = c;

    return new_tac;
}

Tac* join_tac(Tac *tac_1, Tac *tac_2) {
    if (tac_1 == NULL)
        return tac_2;
    if (tac_2 == NULL)
        return tac_1;

    Tac *tac_ptr = tac_2;
    while (tac_ptr->prev != NULL)
        tac_ptr = tac_ptr->prev;
    tac_ptr->prev = tac_1;
    return tac_2;
}

ExprNode* make_node(ExprNode *next, Symbol *result, Tac *code) {
    ExprNode *new_node = malloc(sizeof(ExprNode));
    new_node->next = next;
    new_node->result = result;
    new_node->tac = code;

    return new_node;
}

Tac* tac_assign(Symbol *symb, ExprNode *expr)  {
    if (symb->declaration_type != VAR_TOKEN) {
        printf("Error: assignment to non-variable at line %d\n", symb->line);
        return NULL;
    }

    Tac *code = make_tac(TAC_CPY, symb, expr->result, NULL);
    return code;
}

ExprNode* tac_binop(TacOp bin_op, ExprNode *expr_1, ExprNode *expr_2, SymbolTable *table) {
    if (expr_1->result->token_type == INT_TOKEN && expr_2->result->token_type == INT_TOKEN) {
        switch(bin_op) {
            case TAC_ADD:
                expr_1->result->values->i = expr_1->result->values->i + expr_2->result->values->i;
                break;
            case TAC_SUB:
                expr_1->result->values->i = expr_1->result->values->i - expr_2->result->values->i;
                break;
            case TAC_MULT:
                expr_1->result->values->i = expr_1->result->values->i * expr_2->result->values->i;
                break;
            case TAC_DIV:
                expr_1->result->values->i = expr_1->result->values->i / expr_2->result->values->i;
                break;
            case TAC_MOD:
                expr_1->result->values->i = expr_1->result->values->i % expr_2->result->values->i;
                break;
            case TAC_AND:
                if (expr_1->result->values->i && expr_2->result->values->i)
                    expr_1->result->values->i = 1;
                else
                    expr_1->result->values->i = 0;
                break;
            case TAC_OR:
                if (expr_1->result->values->i || expr_2->result->values->i)
                    expr_1->result->values->i = 1;
                else
                    expr_1->result->values->i = 0;
                break;
        }
        sprintf(expr_1->result->name, "%d", expr_1->result->values->i);
        return expr_1;
    }

    if (expr_1->result->token_type == REAL_TOKEN && expr_2 -> result->token_type == REAL_TOKEN) {
        switch (bin_op) {
            case TAC_ADD:
                expr_1->result->values->f = expr_1->result->values->f + expr_2->result->values->f;
                break;
            case TAC_SUB:
                expr_1->result->values->f = expr_1->result->values->f - expr_2->result->values->f;
                break;
            case TAC_MULT:
                expr_1->result->values->f = expr_1->result->values->f * expr_2->result->values->f;
                break;
            case TAC_DIV:
                expr_1->result->values->f = expr_1->result->values->f / expr_2->result->values->f;
                break;
        }
        sprintf(expr_1->result->name, "%f", expr_1->result->values->f);
        return expr_1;
    }

    if (expr_1->result->token_type == INT_TOKEN && expr_2->result->token_type == REAL_TOKEN) {
        expr_1->result->values->f = expr_1->result->values->i;
        switch (bin_op) {
            case TAC_ADD:
                expr_1->result->values->f = expr_1->result->values->f + expr_2->result->values->f;
                break;
            case TAC_SUB:
                expr_1->result->values->f = expr_1->result->values->f - expr_2->result->values->f;
                break;
            case TAC_MULT:
                expr_1->result->values->f = expr_1->result->values->f * expr_2->result->values->f;
                break;
            case TAC_DIV:
                expr_1->result->values->f = expr_1->result->values->f / expr_2->result->values->f;
                break;
        }
        sprintf(expr_1->result->name, "%f", expr_1->result->values->f);
        return expr_1;
    }

    if (expr_1->result->token_type = REAL_TOKEN && expr_2->result->token_type == INT_TOKEN) {
        expr_2->result->values->f = expr_2->result->values->i;
        switch (bin_op) {
            case TAC_ADD:
                expr_1->result->values->f = expr_1->result->values->f + expr_2->result->values->f;
                break;
            case TAC_SUB:
                expr_1->result->values->f = expr_1->result->values->f - expr_2->result->values->f;
                break;
            case TAC_MULT:
                expr_1->result->values->f = expr_1->result->values->f * expr_2->result->values->f;
                break;
            case TAC_DIV:
                expr_1->result->values->f = expr_1->result->values->f / expr_2->result->values->f;
                break;
        }
        sprintf(expr_1->result->name, "%f", expr_1->result->values->f);
        return expr_1;
    }

    Tac *tmp = make_tac(TAC_VAR, make_tmp(), NULL, NULL);
    tmp->prev = join_tac(expr_1->tac, expr_2->tac);
    Tac *result = make_tac(bin_op, tmp->a.symb, expr_2->result, expr_1->result);
    result->prev = tmp;

    expr_1->result = tmp->a.symb;
    expr_1->tac = result;

    return expr_1;
}
