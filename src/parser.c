#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "scanner.h"
#include "symbol_table.h"
#include "tac.h"

SymbolTable *current_table = NULL;

void syntax_error(const TokenType expected_type) {
    if (current_token != NULL) {
        const char *expected;
        if (expected_type <= KEYWORD_COUNT + SPECIAL_COUNT)
            expected = (expected_type <= KEYWORD_COUNT) ? keywords[expected_type - 1] : specials[expected_type - KEYWORD_COUNT - 1];
        else if (expected_type == ID_TOKEN)
            expected = "identifier";
        else if (expected_type == RVALUE_TOKEN)
            expected = "rvalue";
        else if (expected_type == VTYPE_TOKEN)
            expected = "variable type";
        else if (expected_type == EOF_TOKEN)
            expected = "eof";
        const char *received;
        received = current_token->token;
        if (current_token->type == ID_TOKEN)
            received = "identifier";
        else if (current_token->type == INUM_TOKEN)
            received = "integer number";
        else if (current_token->type == RNUM_TOKEN)
            received = "real number";
        else if (current_token->type == SVAL_TOKEN)
            received = "string literal";
        else if (current_token->type == CVAL_TOKEN)
            received = "character";
        else if (current_token->type == EOF_TOKEN)
            received = "eof";
        printf("Error: expected token %s but got %s at line %d, char %d\n",
            expected, received, current_token->start_ln, current_token->start_col);
    }
}

void type_mismatch_error(const TokenType expected_type, const TokenType received_type) {
    if (current_token != NULL) {
        const char *expected;
        if (expected_type == INUM_TOKEN || expected_type == declaration_value_map(INUM_TOKEN))
            expected = "integer number";
        else if (expected_type == RNUM_TOKEN || expected_type == declaration_value_map(RNUM_TOKEN))
            expected = "real number";
        else if (expected_type == CVAL_TOKEN || expected_type == declaration_value_map(CVAL_TOKEN))
            expected = "character";
        else if (expected_type == SVAL_TOKEN || expected_type == declaration_value_map(SVAL_TOKEN))
            expected = "string literal";
        const char *received;
        if (received_type == INUM_TOKEN || received_type == declaration_value_map(INUM_TOKEN))
            received = "integer number";
        else if (received_type == RNUM_TOKEN || received_type == declaration_value_map(RNUM_TOKEN))
            received = "real number";
        else if (received_type == CVAL_TOKEN || received_type == declaration_value_map(CVAL_TOKEN))
            received = "character";
        else if (received_type == SVAL_TOKEN || received_type == declaration_value_map(SVAL_TOKEN))
            received = "string literal";
        printf("Error: expected type %s but got %s at line %d, char %d\n",
            expected, received, current_token->start_ln, current_token->start_col);
    }
}

int type_check(const TokenType expected_type, const TokenType received_type) {
    if ((int) (expected_type) != -1) {
        if (expected_type == SVAL_TOKEN) {
            if (received_type != SVAL_TOKEN && received_type != declaration_value_map(SVAL_TOKEN) &&
                received_type != CVAL_TOKEN && received_type != declaration_value_map(CVAL_TOKEN))
                return 0;
        }
        else if (expected_type == RNUM_TOKEN) {
            if (received_type != RNUM_TOKEN && received_type != declaration_value_map(RNUM_TOKEN) &&
                received_type != INUM_TOKEN && received_type != declaration_value_map(INUM_TOKEN))
                return 0;
        }
        else if (received_type != expected_type && received_type != declaration_value_map(expected_type))
            return 0;
    }
    return 1;
}

int match(const TokenType type_to_match) {
    if (current_token != NULL && current_token->type == type_to_match)
        return 1;
    return 0;
}

int name_mangle(char **name, int *length, TokenType type, const char *token) { // This is needed for function/procedure overload
    *length += 3;
    char *new_name = malloc(*length);
    if (new_name == NULL) {
        printf("Error: Failed to allocate more memory\n");
        return 0;
    }
    strcpy(new_name, *name);
    free(*name);
    *name = new_name;
    if (type == INT_TOKEN || type == INUM_TOKEN)
        strcat(*name, "_i");
    else if (type == REAL_TOKEN || type == RNUM_TOKEN)
        strcat(*name, "_r");
    else if (type == CHAR_TOKEN || type == CVAL_TOKEN)
        strcat(*name, "_c");
    else if (type == STRING_TOKEN || type == SVAL_TOKEN)
        strcat(*name, "_s");
    else if (type == BOOL_TOKEN || (token != NULL && (!strcmp(token, "true") || !strcmp(token, "false"))))
        strcat(*name, "_b");
    else if ((int) (type) == -1) // For a function/procedure with no args (void)
        strcat(*name, "_v");
    else
        return 0;
    return 1;
}

TokenData *copy_token_data(const TokenData *data) {
    TokenData *cpy_data = malloc(sizeof(TokenData));
    cpy_data->token = malloc(strlen(data->token) + 1);
    strcpy(cpy_data->token, data->token);
    cpy_data->type = data->type;
    cpy_data->start_ln = data->start_ln;
    cpy_data->start_col = data->start_col;

    return cpy_data;
}

int is_value_type() {
    return (match(INUM_TOKEN) || match(RNUM_TOKEN) || match(SVAL_TOKEN) || match(CVAL_TOKEN));
}

int is_variable_type() {
    return (match(INT_TOKEN) || match(REAL_TOKEN) || match(STRING_TOKEN) || match(CHAR_TOKEN) || match(BOOL_TOKEN));
}

int is_logical_op() {
    return (match(PLUS_TOKEN) || match(MINUS_TOKEN) || match(MULT_TOKEN) || match(RDIV_TOKEN));
}

int is_condition_op() {
    return (match(EQ_TOKEN) || match(LESS_TOKEN) || match(LEQ_TOKEN) || match(BIGGER_TOKEN) || match(BEQ_TOKEN) || match(DIFF_TOKEN));
}

Symbol* function_call_check(const TokenData *token_data, const TokenType expected_type) {
    // Same as procedures. Already points on the next token without checking semi-colons
    if (!match(OP_TOKEN)) {
        syntax_error(OP_TOKEN);
        return NULL;
    }
    int param_count = 0;
    int token_len = strlen(token_data->token) + 1;
    char *token = malloc(token_len);
    strcpy(token, token_data->token);
    ParamType *head_param = NULL;
    ParamType **current_param = &head_param;
    do {
        next_token();
        if (match(ID_TOKEN)) {
            TokenData *cpy_token = copy_token_data(current_token);
            next_token();
            if (match(OP_TOKEN)) {
                Symbol *symbol = NULL;
                if ((symbol = function_call_check(cpy_token, -1)) == NULL) {
                    return NULL;
                }
                if (!name_mangle(&token, &token_len, symbol->token_type, cpy_token->token)) {
                    printf("Error: invalid parameter type at line %d, char %d\n", cpy_token->start_ln, cpy_token->start_col);
                    return NULL;
                }
                *current_param = malloc(sizeof(ParamType));
                (*current_param)->param_symbol = make_symbol(symbol->name, symbol->declaration_type, symbol->token_type, cpy_token->start_ln,
                                                        cpy_token->start_col, symbol->dimension, symbol->param_list, symbol->values);
                current_param = &(*current_param)->next;
            }
            else {
                Symbol *symbol = symbol_deep_lookup(cpy_token->token);
                if (symbol == NULL) {
                    printf("Error: identifier not previously declared at line %d, char %d\n",
                        cpy_token->start_ln, cpy_token->start_col);
                    return NULL;
                }
                // printf("Symbol declaration type: %s\nSymbol token type: %s\n", token_type_map[symbol->declaration_type - 1], token_type_map[symbol->token_type - 1]);
                if (!name_mangle(&token, &token_len, symbol->token_type, cpy_token->token)) {
                    printf("Error: invalid parameter type at line %d, char %d\n", cpy_token->start_ln, cpy_token->start_col);
                    return NULL;
                }
                *current_param = malloc(sizeof(ParamType));
                (*current_param)->param_symbol = make_symbol(symbol->name, symbol->declaration_type, symbol->token_type, cpy_token->start_ln,
                                                        cpy_token->start_col, symbol->dimension, symbol->param_list, symbol->values);
                current_param = &(*current_param)->next;
            }
        }
        else {
            if (!is_value_type()) {
                syntax_error(VTYPE_TOKEN);
                return NULL;
            }
            if (!name_mangle(&token, &token_len, current_token->type, current_token->token)) {
                printf("Error: invalid parameter type at line %d, char %d\n", current_token->start_ln, current_token->start_col);
                return NULL;
            }
            *current_param = malloc(sizeof(ParamType));
            (*current_param)->param_symbol = make_symbol(current_token->token, CONST_TOKEN, current_token->type,
                                                    current_token->start_ln, current_token->start_col, 1, NULL, NULL);
            current_param = &(*current_param)->next;
            next_token();
        }
        param_count++;
    } while (match(COMMA_TOKEN));
    if (!match(CP_TOKEN)) {
        syntax_error(CP_TOKEN);
        return NULL;
    }
    if (param_count == 0) {
        name_mangle(&token, &token_len, -1, NULL);
    }
    Symbol *assign_symb = NULL;
    if ((assign_symb = symbol_deep_lookup(token)) == NULL) {
        printf("Error: function/procedure with given parameters not previously declared at line %d, char %d\n", token_data->start_ln,
            token_data->start_col);
        return NULL;
    }
    ParamType *param_list = assign_symb->param_list;
    current_param = &head_param;
    while (param_list != NULL) {
        if (param_list->param_symbol != NULL) {
            TokenType param_type = param_list->param_symbol->declaration_type;
            // printf("\n%s | %s | expected type : %d | given type : %d\n\n", param_list->param_symbol->name, (*current_param)->param_symbol->name,
            //         param_type, (*current_param)->param_symbol->declaration_type);
            if (param_type == VAR_TOKEN && (*current_param)->param_symbol->declaration_type == CONST_TOKEN) {
                printf("Error: expected a variable identifier at line %d, char %d\n", (*current_param)->param_symbol->line,
                    (*current_param)->param_symbol->col);
                return NULL;
            }
        }
        param_list = param_list->next;
        current_param = &(*current_param)->next;
    }
    if ((int)expected_type > 0) {
        if (assign_symb->declaration_type == PROCEDURE_TOKEN) {
            printf("Error: cannot assign procedure to a variable or call it as an argument at line %d, char %d\n",
                token_data->start_ln, token_data->start_col);
            return NULL;
        }
        if (!type_check(expected_type, assign_symb->token_type)) {
            type_mismatch_error(expected_type, assign_symb->token_type);
            return NULL;
        }
    }
    next_token();
    return assign_symb;
}

int parse_program();
int parse_used_libraries();
int parse_constants();
int parse_declarations();
int parse_functions();
int parse_procedures();
int parse_begin(int);
int parse_statement();

int rvalue_statement(TokenType);
int condition_statement();
int assign_statement(const Symbol *);
int if_statement();
int while_statement();
int for_statement();
int write_statement();
int read_statement();

int parse_program() {
    next_token();
    if (current_token != NULL) {
        if (!match(PROGRAM_TOKEN)) {
            syntax_error(PROGRAM_TOKEN);
            return 0;
        }
        init_main_table(); // Initialize main symbol table
        current_table = main_table;
        next_token();
        if (!match(ID_TOKEN)) {
            syntax_error(ID_TOKEN);
            return 0;
        }
        // Symbol *program_id_symb = make_symbol(current_token->token, PROGRAM_TOKEN, 0, current_token->start_ln, current_token->start_col, 0, NULL, NULL);
        next_token();
        if (match(OP_TOKEN)) {
            do {
                next_token();
                if (!match(ID_TOKEN)) {
                    syntax_error(ID_TOKEN);
                    return 0;
                }
                next_token();
            } while (match(COMMA_TOKEN));
            if (!match(CP_TOKEN)) {
                syntax_error(CP_TOKEN);
                return 0;
            }
            next_token();
        }
        if (!match(SC_TOKEN)) {
            syntax_error(SC_TOKEN);
            return 0;
        }
        next_token();
        return parse_used_libraries();
    }

    return 0;
}

int parse_used_libraries() {
    if (!match(USES_TOKEN)) {
        if (match(CONST_TOKEN)) {
            return parse_constants();
        }
        else if (match(VAR_TOKEN)) {
            return parse_declarations();
        }
        else if (match(FUNCTION_TOKEN)) {
            return parse_functions();
        }
        else if (match(PROCEDURE_TOKEN)) {
            return parse_procedures();
        }
        else if (match(BEGIN_TOKEN)) {
            return parse_begin(0);
        }
        else {
            syntax_error(BEGIN_TOKEN);
            return 0;
        }
    }
    do {
        next_token();
        if (!match(ID_TOKEN)) {
            syntax_error(ID_TOKEN);
            return 0;
        }
        int start_col = current_token->start_col;
        Symbol *symb = make_symbol(current_token->token, CONST_TOKEN, SVAL_TOKEN, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
        if (symbol_lookup_insert(main_table, symb) == 1) { // Case where constant already exists in main table
            printf("Error: duplicate library identifier at line %d, char %d\n", symb->line, start_col);
            return 0;
        }
        // Saving the lib name as a string value is redundant ? (No current use, might remove)
        symb->values = make_symbol_value(current_token->token, current_token->type);
        next_token();
    } while (match(COMMA_TOKEN));
    if (!match(SC_TOKEN)) {
        syntax_error(SC_TOKEN);
        return 0;
    }
    next_token();
    return parse_constants();
}

int parse_constants() {
    if (!match(CONST_TOKEN)) {
        if (match(VAR_TOKEN)) {
            return parse_declarations();
        }
        else if (match(FUNCTION_TOKEN)) {
            return parse_functions();
        }
        else if (match(PROCEDURE_TOKEN)) {
            return parse_procedures();
        }
        else if (match(BEGIN_TOKEN)) {
            return parse_begin(0);
        }
        else {
            syntax_error(BEGIN_TOKEN);
            return 0;
        }
    }
    next_token();
    if (!match(ID_TOKEN)) {
        syntax_error(ID_TOKEN);
        return 0;
    }
    do {
        int start_col = current_token->start_col;
        Symbol *symb = make_symbol(current_token->token, CONST_TOKEN, 0, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
        next_token();
        if (!match(EQ_TOKEN)) {
            syntax_error(EQ_TOKEN);
            return 0;
        }
        next_token();
        if (!is_value_type()) {
            syntax_error(RVALUE_TOKEN);
            return 0;
        }
        if (symbol_lookup_insert(main_table, symb) == 1) { // Constant already exists in main table
            printf("Error: unallowed redefinition of a constant at line %d, char %d\n", symb->line, start_col);
            return 0;
        }
        symb->token_type = declaration_value_map(current_token->type);
        symb->values = make_symbol_value(current_token->token, current_token->type);
        next_token();
        if (!match(SC_TOKEN)) {
            syntax_error(SC_TOKEN);
            return 0;
        }
        next_token();
    } while (match(ID_TOKEN));
    return parse_declarations();
}

struct SymbolList { // A linked list structure for the case where multiple variables are declared sharing the same type initialization
    Symbol *symb;
    struct SymbolList *next;
};

int declaration_routine() {
    if (match(VAR_TOKEN)) {
        next_token();
        if (!match(ID_TOKEN)) {
            syntax_error(ID_TOKEN);
            return 0;
        }
        int no_var_init = 0;
        struct SymbolList *list = NULL;
        struct SymbolList *head = NULL;
        while (match(ID_TOKEN)) {
            if (list == NULL) {
                list = malloc(sizeof(struct SymbolList));
            }
            if (head == NULL) {
                head = list;
            }
            list->next = NULL;
            list->symb = make_symbol(current_token->token, VAR_TOKEN, 0, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
            if (symbol_lookup_insert(current_table, list->symb) == 1) {
                printf("Error: duplicate identifier declaration at line %d, char %d\n", current_token->start_ln, current_token->start_col);
                return 0;
            }
            next_token();
            if (match(COMMA_TOKEN)) {
                next_token();
                if (!match(ID_TOKEN)) {
                    syntax_error(ID_TOKEN);
                    return 0;
                }
                list->next = malloc(sizeof(struct SymbolList));
                list->next->symb = make_symbol(current_token->token, VAR_TOKEN, 0, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
                list = list->next;
                no_var_init = 1;
                continue;
            }
            if (!match(COLON_TOKEN)) {
                syntax_error(COLON_TOKEN);
                return 0;
            }
            next_token();
            if (!is_variable_type()) {
                syntax_error(VTYPE_TOKEN);
                return 0;
            }
            Symbol *first_symb = head->symb;
            list = head;
            while (list != NULL) {
                list->symb->token_type = current_token->type;
                struct SymbolList *tmp = list;
                list = list->next;
                free(tmp);
            }
            list = NULL; head = NULL;
            next_token();
            if (no_var_init) {
                if (match(EQ_TOKEN)) {
                    printf("Error: only one variable can be initialized at line %d, char %d\n",
                        current_token->start_ln, current_token->start_col);
                    return 0;
                }
                else if (!match(SC_TOKEN)) {
                    syntax_error(SC_TOKEN);
                    return 0;
                }
            }
            else {
                if (!match(EQ_TOKEN)) {
                    if (!match(SC_TOKEN)) {
                        syntax_error(SC_TOKEN);
                        return 0;
                    }
                }
                else {
                    next_token();
                    if (!is_value_type()) {
                        syntax_error(RVALUE_TOKEN);
                        return 0;
                    }
                    if (!type_check(declaration_value_map(first_symb->token_type), current_token->type)) {
                        type_mismatch_error(declaration_value_map(first_symb->token_type), current_token->type);
                        return 0;
                    }
                    first_symb->values = make_symbol_value(current_token->token, current_token->type);
                    next_token();
                    if (!match(SC_TOKEN)) {
                        syntax_error(SC_TOKEN);
                        return 0;
                    }
                }
            }
            no_var_init = 0;
            next_token();
        }
    }
    return 1; // Return 1 even if it doesn't match a VAR_TOKEN at the start (Tests on VAR_TOKEN should be done before calling this function)
}

int parse_declarations() {
    if (!match(VAR_TOKEN)) {
        if (match(CONST_TOKEN)) {
            return parse_constants();
        }
        else if (match(FUNCTION_TOKEN)) {
            return parse_functions();
        }
        else if (match(PROCEDURE_TOKEN)) {
            return parse_procedures();
        }
        else if (match(BEGIN_TOKEN)) {
            return parse_begin(0);
        }
        else {
            syntax_error(BEGIN_TOKEN);
            return 0;
        }
    }
    if (!declaration_routine()) {
        return 0;
    }
    return parse_functions();
}

int parse_functions() {
    if (!match(FUNCTION_TOKEN)) {
        if (match(CONST_TOKEN)) {
            return parse_constants();
        }
        else if (match(VAR_TOKEN)) {
            return parse_declarations();
        }
        else if (match(PROCEDURE_TOKEN)) {
            return parse_procedures();
        }
        else if (match(BEGIN_TOKEN)) {
            return parse_begin(0);
        }
        else {
            syntax_error(BEGIN_TOKEN);
            return 0;
        }
    }
    next_token();
    if (!match(ID_TOKEN)) {
        syntax_error(ID_TOKEN);
        return 0;
    }
    if (symbol_lookup(main_table, current_token->token) != NULL) {
        printf("Error: identifier %s is not a function at line %d, char %d\n", current_token->token, current_token->start_ln, current_token->start_col);
        return 0;
    }
    Symbol *fid_symb  = make_symbol(current_token->token, FUNCTION_TOKEN, 0, current_token->start_ln, current_token->start_col, 0, NULL, NULL);
    Symbol *copy_symb  = make_symbol(current_token->token, FUNCTION_TOKEN, 0, current_token->start_ln, current_token->start_col, 0, NULL, NULL);
    int fid_length = strlen(current_token->token);
    int start_col = current_token->start_col;
    fid_symb->param_list = malloc(sizeof(ParamType));
    int param_count = 0;
    ParamType **head_param = &(fid_symb->param_list);
    ParamType **current_param = head_param;
    current_table = make_table(current_table);
    symbol_insert(current_table, copy_symb);
    next_token();
    if (match(OP_TOKEN)) {
        int id_found = 0, ref_pass = 0;
        do {
            next_token();
            if (match(VAR_TOKEN)) { // Case of passing an argument by reference
                id_found = ref_pass = 1;
                next_token();
            }
            if (id_found && !match(ID_TOKEN)) {
                syntax_error(ID_TOKEN);
                return 0;
            }
            if (match(ID_TOKEN)) {
                id_found = 1;
                param_count++;
                TokenType param_type = ref_pass ? VAR_TOKEN : CONST_TOKEN;
                Symbol *new_psymb = make_symbol(current_token->token, param_type, 0, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
                if (*current_param == NULL) {
                    *current_param = malloc(sizeof(ParamType));
                }
                // printf("\n%s\n\n", current_param->param_symbol->name);
                (*current_param)->param_symbol = new_psymb;
                (*current_param)->ref_pass = ref_pass;
                (*current_param)->next = NULL;
                if (symbol_lookup_insert(current_table, new_psymb) == 1) {
                    printf("Error: duplicate parameter identifier at line %d, char %d\n", current_token->start_ln, current_token->start_col);
                    return 0;
                }
                next_token();
                while (match(COMMA_TOKEN)) {
                    next_token();
                    if (!match(ID_TOKEN)) {
                        syntax_error(ID_TOKEN);
                        return 0;
                    }
                    param_count++;
                    (*current_param)->next = malloc(sizeof(ParamType));
                    new_psymb = make_symbol(current_token->token, param_type, 0, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
                    (*current_param)->next->param_symbol = new_psymb;
                    (*current_param)->next->ref_pass = ref_pass;
                    (*current_param)->next->next = NULL;
                    current_param = &(*current_param)->next;
                    if (symbol_lookup_insert(current_table, new_psymb) == 1) {
                        printf("Error: duplicate parameter identifier at line %d, char %d\n", current_token->start_ln, current_token->start_col);
                        return 0;
                    }
                    next_token();
                }
                current_param = &(*current_param)->next;
                if (!match(COLON_TOKEN)) {
                    syntax_error(COLON_TOKEN);
                    return 0;
                }
                next_token();
                if (!is_variable_type()) {
                    syntax_error(VTYPE_TOKEN);
                    return 0;
                }
                while (*head_param != NULL) {
                    (*head_param)->param_symbol->token_type = current_token->type;
                    if (!name_mangle(&(fid_symb->name), &fid_length, current_token->type, NULL)) {
                        return 0;
                    }
                    head_param = &(*head_param)->next;
                }
                ref_pass = 0;
                next_token();
            }
        } while (id_found && match(SC_TOKEN));
        if (!match(CP_TOKEN)) {
            syntax_error(CP_TOKEN);
            return 0;
        }
        next_token();
    }
    // printf("\n%s\n\n", fid_symb->name);
    if (param_count == 0) {
        if (!name_mangle(&(fid_symb->name), &fid_length, -1, NULL)) {
            return 0;
        }
    }
    if (symbol_lookup_insert(main_table, fid_symb) == 1) {
        printf("Error: overloaded function has the same parameter list at line %d, char %d\n", fid_symb->line, start_col);
        return 0;
    }
    if (!match(COLON_TOKEN)) {
        syntax_error(COLON_TOKEN);
        return 0;
    }
    next_token();
    if (!is_variable_type()) {
        syntax_error(VTYPE_TOKEN);
        return 0;
    }
    fid_symb->token_type = current_token->type;
    copy_symb->token_type = current_token->type;
    fid_symb->dimension = param_count;
    copy_symb->dimension = param_count;
    next_token();
    if (!match(SC_TOKEN)) {
        syntax_error(SC_TOKEN);
        return 0;
    }
    next_token();
    if (!declaration_routine()) {
        return 0;
    }
    if (!parse_begin(1)) {
        return 0;
    }
    if (!match(END_TOKEN)) {
        syntax_error(END_TOKEN);
        return 0;
    }
    next_token();
    if (!match(SC_TOKEN)) {
        syntax_error(SC_TOKEN);
        return 0;
    }
    next_token();
    return parse_functions();
}

int parse_procedures() {
    if (!match(PROCEDURE_TOKEN)) {
        if (match(CONST_TOKEN)) {
            return parse_constants();
        }
        else if (match(VAR_TOKEN)) {
            return parse_declarations();
        }
        else if (match(FUNCTION_TOKEN)) {
            return parse_functions();
        }
        else if (match(BEGIN_TOKEN)) {
            return parse_begin(0);
        }
        else {
            syntax_error(BEGIN_TOKEN);
            return 0;
        }
    }
    next_token();
    if (!match(ID_TOKEN)) {
        syntax_error(ID_TOKEN);
        return 0;
    }
    if (symbol_lookup(main_table, current_token->token) != NULL) {
        printf("Error: identifier %s is not a procedure at line %d, char %d\n", current_token->token, current_token->start_ln, current_token->start_col);
        return 0;
    }
    Symbol *pid_symb  = make_symbol(current_token->token, PROCEDURE_TOKEN, 0, current_token->start_ln, current_token->start_col, 0, NULL, NULL);
    int pid_length = strlen(current_token->token);
    int start_col = current_token->start_col;
    pid_symb->param_list = NULL;
    int param_count = 0;
    ParamType **head_param = &(pid_symb->param_list);
    ParamType **current_param = head_param;
    current_table = make_table(current_table);
    next_token();
    if (match(OP_TOKEN)) {
        int id_found = 0, ref_pass = 0;
        do {
            next_token();
            if (match(VAR_TOKEN)) { // Case of passing an argument by reference
                id_found = ref_pass = 1;
                next_token();
            }
            if (id_found && !match(ID_TOKEN)) {
                syntax_error(ID_TOKEN);
                return 0;
            }
            if (match(ID_TOKEN)) {
                id_found = 1;
                param_count++;
                TokenType param_type = ref_pass ? VAR_TOKEN : CONST_TOKEN;
                Symbol *new_psymb = make_symbol(current_token->token, param_type, 0, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
                if (*current_param == NULL) {
                    *current_param = malloc(sizeof(ParamType));
                }
                // printf("\n%s\n\n", current_param->param_symbol->name);
                (*current_param)->param_symbol = new_psymb;
                (*current_param)->ref_pass = ref_pass;
                (*current_param)->next = NULL;
                if (symbol_lookup_insert(current_table, new_psymb) == 1) {
                    printf("Error: duplicate parameter identifier at line %d, char %d\n", current_token->start_ln, current_token->start_col);
                    return 0;
                }
                next_token();
                while (match(COMMA_TOKEN)) {
                    next_token();
                    if (!match(ID_TOKEN)) {
                        syntax_error(ID_TOKEN);
                        return 0;
                    }
                    param_count++;
                    (*current_param)->next = malloc(sizeof(ParamType));
                    new_psymb = make_symbol(current_token->token, param_type, 0, current_token->start_ln, current_token->start_col, 1, NULL, NULL);
                    (*current_param)->next->param_symbol = new_psymb;
                    (*current_param)->next->ref_pass = ref_pass;
                    (*current_param)->next->next = NULL;
                    current_param = &(*current_param)->next;
                    if (symbol_lookup_insert(current_table, new_psymb) == 1) {
                        printf("Error: duplicate parameter identifier at line %d, char %d\n", current_token->start_ln, current_token->start_col);
                        return 0;
                    }
                    next_token();
                }
                current_param = &(*current_param)->next;
                if (!match(COLON_TOKEN)) {
                    syntax_error(COLON_TOKEN);
                    return 0;
                }
                next_token();
                if (!is_variable_type()) {
                    syntax_error(VTYPE_TOKEN);
                    return 0;
                }
                while (*head_param != NULL) {
                    (*head_param)->param_symbol->token_type = current_token->type;
                    if (!name_mangle(&(pid_symb->name), &pid_length, current_token->type, NULL)) {
                        return 0;
                    }
                    head_param = &(*head_param)->next;
                }
                ref_pass = 0;
                next_token();
            }
        } while (id_found && match(SC_TOKEN));
        if (!match(CP_TOKEN)) {
            syntax_error(CP_TOKEN);
            return 0;
        }
        next_token();
    }
    // printf("\n%s\n\n", pid_symb->name);
    if (param_count == 0) {
        if (!name_mangle(&(pid_symb->name), &pid_length, -1, NULL)) {
            return 0;
        }
    }
    if (symbol_lookup_insert(main_table, pid_symb) == 1) {
        printf("Error: overloaded procedure has the same parameter list at line %d, char %d\n", pid_symb->line, start_col);
        return 0;
    }
    pid_symb->dimension = param_count;
    if (!match(SC_TOKEN)) {
        syntax_error(SC_TOKEN);
        return 0;
    }
    next_token();
    if (!declaration_routine()) {
        return 0;
    }
    if (!parse_begin(1)) {
        return 0;
    }
    if (!match(END_TOKEN)) {
        syntax_error(END_TOKEN);
        return 0;
    }
    next_token();
    if (!match(SC_TOKEN)) {
        syntax_error(SC_TOKEN);
        return 0;
    }
    next_token();
    return parse_procedures();
}

int parse_begin(int no_end_check) {
    if (!match(BEGIN_TOKEN)) {
        syntax_error(BEGIN_TOKEN);
        return 0;
    }
    next_token();
    if (current_token == NULL) {
        return 0;
    }
    int parse_result = 0;
    while (!match(END_TOKEN) && (parse_result = parse_statement())) {
        if (current_token == NULL) {
            return 0;
        }
        if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) {
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
        else {
            next_token();
        }
    }
    if (!no_end_check) {
        if (match(END_TOKEN)) {
            next_token();
            if (!match(PERIOD_TOKEN)) {
                syntax_error(PERIOD_TOKEN);
                return 0;
            }
            next_token();
            if (!match(EOF_TOKEN)) {
                syntax_error(EOF_TOKEN);
                return 0;
            }
        }
        else if (parse_result) {
            syntax_error(END_TOKEN);
            return 0;
        }
    }
    if (!parse_result) {
        return 0;
    }
    return 1;
}

int parse_statement() { // Doesn't include a semicolon check and already points on to the next token
    if (match(ID_TOKEN)) {
        TokenData *start_token = copy_token_data(current_token);
        next_token();
        if (match(ASSIGN_TOKEN)) {
            Symbol *assign_symb;
            if ((assign_symb = symbol_deep_lookup(start_token->token)) == NULL) {
                printf("Error: identifier \"%s\" not previously declared at line %d, char %d\n", current_token->token,
                    current_token->start_ln, current_token->start_col);
                free(start_token->token);
                free(start_token);
                return 0;
            }
            if (assign_symb->declaration_type == CONST_TOKEN) {
                printf("Error: expected variable identifier but got constant at line %d, char %d\n", current_token->start_ln,
                    current_token->start_col);
                free(start_token->token);
                free(start_token);
                return 0;
            }
            return assign_statement(assign_symb);
        }
        else if (match(OP_TOKEN)) {
            return function_call_check(start_token, -1) != NULL ? 1 : 0;
        }
        return 0;
    }
    if (match(IF_TOKEN))
        return if_statement();
    if (match(WHILE_TOKEN))
        return while_statement();
    if (match(FOR_TOKEN))
        return for_statement();
    if (match(WRITE_TOKEN) || match(WRITELN_TOKEN))
        return write_statement();
    if (match(READ_TOKEN))
        return read_statement();
    if (!match(EOF_TOKEN))
        printf("Error: illegal expression at line %d, char %d\n", current_token->start_ln, current_token->start_col);
    return 0;
}

int rvalue_statement(const TokenType expected_type) {
    Symbol *tmp = NULL;
    do {
        next_token();
        if (!match(ID_TOKEN)) {
            if (match(OP_TOKEN)) {
                do {
                    next_token();
                    if (match(ID_TOKEN)) {
                        TokenData *token_data = copy_token_data(current_token);
                        next_token();
                        if (match(OP_TOKEN)) { // Case of a function/procedure call (Note: procedures cannot be assigned to a variable or called as an argument)
                            if (!function_call_check(token_data, expected_type)) {
                                free(token_data->token);
                                free(token_data);
                                return 0;
                            }
                        }
                        else {
                            tmp = symbol_deep_lookup(token_data->token);
                            if (tmp == NULL) {
                                printf("Error: identifier \"%s\" not previously declared at line %d, char %d\n", token_data->token, 
                                    token_data->start_ln, token_data->start_col);
                                free(token_data->token);
                                free(token_data);
                                return 0;
                            }
                            if (!type_check(expected_type, tmp->token_type)) {
                                type_mismatch_error(expected_type, tmp->token_type);
                                free(token_data->token);
                                free(token_data);
                                return 0;
                            }
                        }
                        free(token_data->token);
                        free(token_data);
                    }
                    else if (is_value_type()) {
                        if (!type_check(expected_type, current_token->type)) {
                            type_mismatch_error(expected_type, current_token->type);
                            return 0;
                        }
                        next_token();
                    }
                    else {
                        printf("Error: expected token identifier or rvalue but got %s at line %d, char %d\n",
                            current_token->token, current_token->start_ln, current_token->start_col);
                        return 0;
                    }
                } while(is_logical_op());
                if (!match(CP_TOKEN)) {
                    syntax_error(CP_TOKEN);
                    return 0;
                }
            }
            else if (!is_value_type()) {
                printf("Error: expected token identifier or rvalue but got %s at line %d, char %d\n",
                    current_token->token, current_token->start_ln, current_token->start_col);
                return 0;
            }
            else if (!type_check(expected_type, current_token->type)) {
                type_mismatch_error(expected_type, current_token->type);
                return 0;
            }
            next_token();
        }
        else { // Identifier token read
            TokenData *token_data = copy_token_data(current_token);
            next_token();
            if (match(OP_TOKEN)) { // Function or procedure call
                if (!function_call_check(token_data, expected_type)) {
                    free(token_data->token);
                    free(token_data);
                    return 0;
                }
            }
            else {
                tmp = symbol_deep_lookup(token_data->token);
                if (tmp == NULL) {
                    printf("Error: identifier \"%s\" not previously declared at line %d, char %d\n", token_data->token, 
                        token_data->start_ln, token_data->start_col);
                    free(token_data->token);
                    free(token_data);
                    return 0;
                }
                if (!type_check(expected_type, tmp->token_type)) {
                    type_mismatch_error(expected_type, tmp->token_type);
                    free(token_data->token);
                    free(token_data);
                    return 0;
                }
            }
            free(token_data->token);
            free(token_data);
        }
    } while (is_logical_op());
    return 1;
}

int condition_statement() {
    if (!match(ID_TOKEN) && !is_value_type()) {
        if (match(NOT_TOKEN))
            next_token();
        if (!match(OP_TOKEN)) {
            printf("Error: expected condition statement but got %s at line %d, char %d\n",
                current_token->token, current_token->start_ln, current_token->start_col);
            return 0;
        }
        while (match(OP_TOKEN)) {
            next_token();
            if (!match(ID_TOKEN) && !is_value_type()) {
                printf("Error: expected token identifier or rvalue but got %s at line %d, char %d\n",
                    current_token->token, current_token->start_ln, current_token->start_col);
                return 0;
            }
            next_token();
            if (!is_condition_op()) {
                printf("Error: expected logical operator but got %s at line %d, char %d\n",
                    current_token->token, current_token->start_ln, current_token->start_col);
                return 0;
            }
            next_token();
            if (!match(ID_TOKEN) && !is_value_type()) {
                printf("Error: expected token identifier or rvalue but got %s at line %d, char %d\n",
                    current_token->token, current_token->start_ln, current_token->start_col);
                return 0;
            }
            next_token();
            if (!match(CP_TOKEN)) {
                syntax_error(CP_TOKEN);
                return 0;
            }
            next_token();
            if (match(AND_TOKEN) || match(OR_TOKEN))
                next_token();
        }
        return 1;
    }
    next_token();
    if (!is_condition_op()) {
        printf("Error: expected logical operator but got %s at line %d, char %d\n",
            current_token->token, current_token->start_ln, current_token->start_col);
        return 0;
    }
    next_token();
    if (!match(ID_TOKEN) && !is_value_type()) {
        printf("Error: expected token identifier or number or string literal but got %s at line %d, char %d\n",
            current_token->token, current_token->start_ln, current_token->start_col);
        return 0;
    }
    next_token();
    return 1;
}

int assign_statement(const Symbol *assign_symb) {
    if (!match(ASSIGN_TOKEN)) {
        syntax_error(ASSIGN_TOKEN);
        return 0;
    }
    if (!rvalue_statement(assign_symb->token_type))
        return 0;
    return 1;
}

int if_statement() {
    if (!match(IF_TOKEN)) {
        syntax_error(IF_TOKEN);
        return 0;
    }
    next_token();
    if (!condition_statement())
        return 0;
    if (!match(THEN_TOKEN)) {
        syntax_error(THEN_TOKEN);
        return 0;
    }
    next_token();
    if (!match(BEGIN_TOKEN)) {
        if (!parse_statement())
            return 0;
        if (match(ELSE_TOKEN)) {
            next_token();
            if (match(IF_TOKEN)) // Reminder that this is a case of an else if statement
                if_statement();
            else if (!parse_statement())
                return 0;
            if (!match(SC_TOKEN)) {
                if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                    syntax_error(SC_TOKEN);
                    return 0;
                }
            }
        }
        else if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
        return 1;
    }
    next_token();
    while (!match(END_TOKEN) && parse_statement()) {
        if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
        else
            next_token();
    }
    if (!match(END_TOKEN)) {
        syntax_error(END_TOKEN);
        return 0;
    }
    next_token();
    if (match(ELSE_TOKEN)) {
        next_token();
        if (match(IF_TOKEN)) // Reminder that this is a case of an else if statement
            if_statement();
        else if (!parse_statement())
            return 0;
        if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
    }
    else if (!match(SC_TOKEN)) {
        if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
            syntax_error(SC_TOKEN);
            return 0;
        }
    }
    return 1;
}

int while_statement() {
    if (!match(WHILE_TOKEN)) {
        syntax_error(WHILE_TOKEN);
        return 0;
    }
    next_token();
    if (!condition_statement())
        return 0;
    if (!match(DO_TOKEN)) {
        syntax_error(DO_TOKEN);
        return 0;
    }
    next_token();
    if (!match(BEGIN_TOKEN)) {
        if (!parse_statement())
            return 0;
        if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
        return 1;
    }
    next_token();
    while (!match(END_TOKEN) && parse_statement()) {
        if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
        else
            next_token();
    }
    if (!match(END_TOKEN)) {
        syntax_error(END_TOKEN);
        return 0;
    }
    next_token();
    if (!match(SC_TOKEN)) {
        if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
            syntax_error(SC_TOKEN);
            return 0;
        }
    }
    return 1;
}

int for_statement() {
    if (!match(FOR_TOKEN)) {
        syntax_error(FOR_TOKEN);
        return 0;
    }
    next_token();
    if (!match(ID_TOKEN)) {
        syntax_error(ID_TOKEN);
        return 0;
    }
    Symbol *assign_symb = symbol_deep_lookup(current_token->token);
    if (assign_symb == NULL) {
        printf("Error: identifier not previously declared at line %d, char %d\n", current_token->start_ln, current_token->start_col);
        return 0;
    }
    next_token();
    if (!match(ASSIGN_TOKEN)) {
        syntax_error(ASSIGN_TOKEN);
        return 0;
    }
    if (!rvalue_statement(assign_symb->token_type))
        return 0;
    if (!match(TO_TOKEN) && !match(DOWNTO_TOKEN)) {
        syntax_error(TO_TOKEN);
        return 0;
    }
    if (!rvalue_statement(assign_symb->token_type))
        return 0;
    if (!match(DO_TOKEN)) {
        syntax_error(DO_TOKEN);
        return 0;
    }
    next_token();
    if (!match(BEGIN_TOKEN)) {
        if (!parse_statement())
            return 0;
        if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
        return 1;
    }
    next_token();
    while (!match(END_TOKEN) && parse_statement()) {
        if (!match(SC_TOKEN)) {
            if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
                syntax_error(SC_TOKEN);
                return 0;
            }
        }
        else
            next_token();
    }
    if (!match(END_TOKEN)) {
        syntax_error(END_TOKEN);
        return 0;
    }
    next_token();
    if (!match(SC_TOKEN)) {
        if (!match(END_TOKEN)) { // else case is where we neglect semicolon at the last statement before and end token (OK in Pascal)
            syntax_error(SC_TOKEN);
            return 0;
        }
    }
    return 1;
}

int write_statement() {
    if (!match(WRITE_TOKEN) && !match(WRITELN_TOKEN)) {
        syntax_error(WRITE_TOKEN);
        return 0;
    }
    if (!rvalue_statement(-1)) // I should check what types are compatible with the write and writeln functions
        return 0;
    return 1;
}

int read_statement() {
    if (!match(READ_TOKEN)) {
        syntax_error(READ_TOKEN);
        return 0;
    }
    if (!rvalue_statement(-1)) // I should check what types are compatible with the read function
        return 0;
    return 1;
}

int parse_file(const char *path) {
    if (open_target_file(path)) {
        int result = parse_program();
        close_target_file();
        return result;
    }
    return 0;
}
