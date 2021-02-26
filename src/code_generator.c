#include <stdio.h>

#include "tac.h"
#include "code_generator.h"

void process_instruction(Tac *tac) {
    switch (tac->op) {
        case TAC_UNDEF: {
            break;
        }
        case TAC_ADD: {
            break;
        }
        case TAC_SUB: {
            break;
        }
        case TAC_MULT: {
            break;
        }
        case TAC_DIV: {
            break;
        }
        default: {
            break;
        }
    }
}

void generate_code(Tac *head_tac) {
    if (head_tac != NULL) {
        Tac *next = NULL;
        Tac *prev = head_tac;

        while (prev != NULL) {
            prev->next = next;
            next = prev;
            prev = prev->prev;
        }

        Tac *current_tac = next->next;
        while (current_tac != NULL) {
            process_instruction(current_tac);
            current_tac = current_tac->next;
        }
    }
}