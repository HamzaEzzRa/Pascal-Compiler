#include <stdio.h>
#include <stdlib.h>

#include "scanner.h"
#include "symbol_table.h"
#include "tac.h"

int label_idx = 0;

Tac* make_tac(TacOp op, Symbol *a, Symbol *b, Symbol *c) {
    Tac *new_tac = malloc(sizeof(new_tac));
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
    while (tac_ptr->prev != NULL) {
        tac_ptr = tac_ptr->prev;
    }

    tac_ptr->prev = tac_1;
    return tac_2;
}

ExprNode* make_node(ExprNode *next, Symbol *result, Tac *code) {
    ExprNode *new_node = malloc(sizeof(new_node));
    new_node->next = next;
    new_node->result = result;
    new_node->tac = code;

    return new_node;
}

Tac* tac_declare(Symbol *symb) {
    Tac *tac = make_tac(TAC_VAR, symb, NULL, NULL);
}

Tac* tac_assign(Symbol *symb, ExprNode *expr)  {
    if (symb->declaration_type != VAR_TOKEN) {
        printf("Error: assignment to non-variable at line %d\n", symb->line);
        return NULL;
    }

    Tac *code = make_tac(TAC_CPY, symb, expr->result, NULL);
    return code;
}

ExprNode* tac_unary_op(TacOp op, ExprNode *expr) {
    if (expr->result->token_type == INT_TOKEN) {
        switch (op) {
            case TAC_POS: {
                if (expr->result->values->i < 0) {
                    expr->result->values->i *= -1;
                }
                break;
            }
            case TAC_NEG: {
                if (expr->result->values->i > 0) {
                    expr->result->values->i *= -1;
                }
                break;
            }
            case TAC_NOT: {
                expr->result->values->i = (expr->result->values->i == 0);
                break;
            }
        }
        return expr;
    }

    if (expr->result->token_type == REAL_TOKEN) {
        switch (op) {
            case TAC_POS: {
                if (expr->result->values->f < 0) {
                    expr->result->values->f *= -1;
                }
                break;
            }
            case TAC_NEG: {
                if (expr->result->values->f > 0) {
                    expr->result->values->f *= -1;
                }
                break;
            }
            case TAC_NOT: {
                expr->result->values->f = (expr->result->values->f == 0);
                break;
            }
        }
        return expr;
    }

    Symbol *tmp_symb = make_symbol(NULL, 0, 0, 0, 0, 0, NULL, NULL);

    Tac *tmp = make_tac(TAC_VAR, tmp_symb, NULL, NULL);
    tmp->prev = expr->tac;
    Tac *result = make_tac(op, tmp->a.symb, NULL, expr->result);
    result->prev = tmp;

    expr->result = tmp->a.symb;
    expr->tac = result;

    return expr;
}

ExprNode* tac_binary_op(TacOp op, ExprNode *expr_1, ExprNode *expr_2) {
    if (expr_1->result->token_type == INT_TOKEN && expr_2->result->token_type == INT_TOKEN) {
        switch (op) {
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
        switch (op) {
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
        switch (op) {
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
        switch (op) {
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

    Symbol *tmp_symb = make_symbol(NULL, 0, 0, 0, 0, 0, NULL, NULL);

    Tac *tmp = make_tac(TAC_VAR, tmp_symb, NULL, NULL);
    tmp->prev = join_tac(expr_1->tac, expr_2->tac);
    Tac *result = make_tac(op, tmp->a.symb, expr_2->result, expr_1->result);
    result->prev = tmp;

    expr_1->result = tmp->a.symb;
    expr_1->tac = result;

    return expr_1;
}

ExprNode* tac_relation_op(TacOp op, ExprNode *expr_1, ExprNode *expr_2) {
    if (expr_1->result->declaration_type == CONST_TOKEN && expr_2->result->declaration_type == CONST_TOKEN) {
        if (expr_1->result->token_type == INT_TOKEN && expr_2->result->token_type == INT_TOKEN) {
            switch (op) {
                case TAC_EQ: {
                    expr_1->result->values->i = (expr_1->result->values->i == expr_2->result->values->i);
                    break;
                }
                case TAC_NEQ: {
                    expr_1->result->values->i = (expr_1->result->values->i != expr_2->result->values->i);
                    break;
                }
                case TAC_LT: {
                    expr_1->result->values->i = (expr_1->result->values->i < expr_2->result->values->i);
                    break;
                }
                case TAC_LTE: {
                    expr_1->result->values->i = (expr_1->result->values->i <= expr_2->result->values->i);
                    break;
                }
                case TAC_GT: {
                    expr_1->result->values->i = (expr_1->result->values->i > expr_2->result->values->i);
                    break;
                }
                case TAC_GTE: {
                    expr_1->result->values->i = (expr_1->result->values->i >= expr_2->result->values->i);
                    break;
                }
            }
            sprintf(expr_1->result->name, "%d", expr_1->result->values->i);
            return expr_1;
        }
        if (expr_1->result->token_type == REAL_TOKEN && expr_2->result->token_type == REAL_TOKEN) {
            switch (op) {
                case TAC_EQ: {
                    expr_1->result->values->f = (expr_1->result->values->f == expr_2->result->values->f);
                    break;
                }
                case TAC_NEQ: {
                    expr_1->result->values->f = (expr_1->result->values->f != expr_2->result->values->f);
                    break;
                }
                case TAC_LT: {
                    expr_1->result->values->f = (expr_1->result->values->f < expr_2->result->values->f);
                    break;
                }
                case TAC_LTE: {
                    expr_1->result->values->f = (expr_1->result->values->f <= expr_2->result->values->f);
                    break;
                }
                case TAC_GT: {
                    expr_1->result->values->f = (expr_1->result->values->f > expr_2->result->values->f);
                    break;
                }
                case TAC_GTE: {
                    expr_1->result->values->f = (expr_1->result->values->f >= expr_2->result->values->f);
                    break;
                }
            }
            sprintf(expr_1->result->name, "%f", expr_1->result->values->f);
            return expr_1;
        }
        if (expr_1->result->token_type == REAL_TOKEN && expr_2->result->token_type == INT_TOKEN) {
            expr_2->result->values->f = expr_2->result->values->i;
            switch (op) {
                case TAC_EQ: {
                    expr_1->result->values->f = (expr_1->result->values->f == expr_2->result->values->f);
                    break;
                }
                case TAC_NEQ: {
                    expr_1->result->values->f = (expr_1->result->values->f != expr_2->result->values->f);
                    break;
                }
                case TAC_LT: {
                    expr_1->result->values->f = (expr_1->result->values->f < expr_2->result->values->f);
                    break;
                }
                case TAC_LTE: {
                    expr_1->result->values->f = (expr_1->result->values->f <= expr_2->result->values->f);
                    break;
                }
                case TAC_GT: {
                    expr_1->result->values->f = (expr_1->result->values->f > expr_2->result->values->f);
                    break;
                }
                case TAC_GTE: {
                    expr_1->result->values->f = (expr_1->result->values->f >= expr_2->result->values->f);
                    break;
                }
            }
            sprintf(expr_1->result->name, "%f", expr_1->result->values->f);
            return expr_1;
        }
        if (expr_1->result->token_type == INT_TOKEN && expr_2->result->token_type == REAL_TOKEN) {
            expr_1->result->values->f = expr_1->result->values->i;
            switch (op) {
                case TAC_EQ: {
                    expr_1->result->values->f = (expr_1->result->values->f == expr_2->result->values->f);
                    break;
                }
                case TAC_NEQ: {
                    expr_1->result->values->f = (expr_1->result->values->f != expr_2->result->values->f);
                    break;
                }
                case TAC_LT: {
                    expr_1->result->values->f = (expr_1->result->values->f < expr_2->result->values->f);
                    break;
                }
                case TAC_LTE: {
                    expr_1->result->values->f = (expr_1->result->values->f <= expr_2->result->values->f);
                    break;
                }
                case TAC_GT: {
                    expr_1->result->values->f = (expr_1->result->values->f > expr_2->result->values->f);
                    break;
                }
                case TAC_GTE: {
                    expr_1->result->values->f = (expr_1->result->values->f >= expr_2->result->values->f);
                    break;
                }
            }
            sprintf(expr_1->result->name, "%f", expr_1->result->values->f);
            return expr_1;
        }
    }

    Symbol *tmp_symb = make_symbol(NULL, 0, 0, 0, 0, 0, NULL, NULL);

    Tac *tmp = make_tac(TAC_VAR, tmp_symb, NULL, NULL);
    tmp->prev = join_tac(expr_1->tac, expr_2->tac);
    Tac *result = make_tac(op, tmp->a.symb, expr_2->result, expr_1->result);
    result->prev = tmp;

    expr_1->result = tmp->a.symb;
    expr_1->tac = result;

    return expr_1;
}

Symbol* make_label(int value) {
    SymbolValue *new_value = malloc(sizeof(SymbolValue));
    new_value->i = label_idx;
    Symbol *new_label = make_symbol(NULL, 0, LABEL_TOKEN, 0, 0, 0, NULL, new_value);

    return new_label;
}

Tac* tac_program(Symbol *func, Symbol *args, Tac *code) {
    Tac *label = make_tac(TAC_LABEL, make_label(label_idx++), NULL, NULL);
    Tac *begin = make_tac(TAC_BEGINPROC, func, NULL, NULL);
    Tac *end = make_tac(TAC_ENDPROC, NULL, NULL, NULL);
    Tac *arg_list = make_tac(TAC_ARGLIST, args, NULL, NULL);

    begin->prev = label;
    code = join_tac(arg_list, code);
    end->prev = join_tac(begin, code);

    return end;
}

ExprNode *tac_function(Symbol *func, ExprNode *args) {

}

ExprNode *tac_procedure(Symbol *proc, ExprNode *args) {
    
}

Tac* tac_if(ExprNode *expr, Tac *statement) {
    Tac *label = make_tac(TAC_LABEL, make_label(label_idx++), NULL, NULL);
    Tac *code = make_tac(TAC_IFZ, label->a.symb, expr->result, NULL);

    code->prev = expr->tac;
    code = join_tac(code, statement);
    label->prev = code;

    return label;
}

Tac* tac_while(ExprNode *expr, Tac *statement) {
    Tac *label = make_tac(TAC_LABEL, make_label(label_idx++), NULL, NULL);
    Tac *code = make_tac(TAC_GOTO, label->a.symb, NULL, NULL);

    code->prev = statement;
    return join_tac(label, tac_if(expr, code));
}

Tac* tac_print(ExprNode *expr) {
    Tac *code;

    code = make_tac(TAC_PRINT, expr->result, NULL, NULL);
    code->prev = expr->tac;

    return code;
}