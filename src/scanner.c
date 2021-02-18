#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "scanner.h"

const char* const keywords[] = {
    "and", "array", "asm", "begin", "boolean", "break", "case", "char", "const", "constructor", "continue", "destructor", "div", "do", "downto",
    "else", "end", "file", "for", "function", "goto", "if", "implementation", "in", "inline", "integer", "interface", "label", "mod", "nil",
    "not", "object", "of", "on", "operator", "or", "packed", "procedure", "program", "read", "real", "record", "repeat", "set", "shl", "shr",
    "string", "then", "to", "type", "unit", "until", "uses", "var", "while", "with", "write", "writeln", "xor"
};

const char* const specials[] = { ";", ":", ".", "+", "-", "*", "/", ",", ":=", "=", "<", "<=", ">", ">=", "<>", "(", ")" };

const char* const token_type_map[] = {
    "AND_TOKEN", "ARRAY_TOKEN", "ASM_TOKEN", "BEGIN_TOKEN", "BOOL_TOKEN", "BREAK_TOKEN", "CASE_TOKEN", "CHAR_TOKEN", "CONST_TOKEN",
    "CONSTRUCTOR_TOKEN", "CONTINUE_TOKEN", "DESTRUCTOR_TOKEN", "IDIV_TOKEN", "DO_TOKEN", "DOWNTO_TOKEN", "ELSE_TOKEN", "END_TOKEN", "FILE_TOKEN",
    "FOR_TOKEN", "FUNCTION_TOKEN", "GOTO_TOKEN", "IF_TOKEN", "IMPLEMENTATION_TOKEN", "IN_TOKEN", "INLINE_TOKEN", "INT_TOKEN", "INTERFACE_TOKEN",
    "LABEL_TOKEN", "MOD_TOKEN", "NIL_TOKEN", "NOT_TOKEN", "OBJECT_TOKEN", "OF_TOKEN", "ON_TOKEN", "OPERATOR_TOKEN", "OR_TOKEN", "PACKED_TOKEN",
    "PROCEDURE_TOKEN", "PROGRAM_TOKEN", "READ_TOKEN", "REAL_TOKEN", "RECORD_TOKEN", "REPEAT_TOKEN", "SET_TOKEN", "SHL_TOKEN", "SHR_TOKEN",
    "STRING_TOKEN", "THEN_TOKEN", "TO_TOKEN", "TYPE_TOKEN", "UNIT_TOKEN", "UNTIL_TOKEN", "USES_TOKEN", "VAR_TOKEN", "WHILE_TOKEN", "WITH_TOKEN",
    "WRITE_TOKEN", "WRITELN_TOKEN", "XOR_TOKEN", "SC_TOKEN", "COLON_TOKEN", "PERIOD_TOKEN", "PLUS_TOKEN", "MINUS_TOKEN", "MULT_TOKEN",
    "RDIV_TOKEN", "COMMA_TOKEN", "ASSIGN_TOKEN", "EQ_TOKEN", "LESS_TOKEN", "LEQ_TOKEN", "BIGGER_TOKEN", "BEQ_TOKEN", "DIFF_TOKEN", "OP_TOKEN",
    "CP_TOKEN", "EOF_TOKEN", "ID_TOKEN", "INUM_TOKEN", "RNUM_TOKEN", "SVAL_TOKEN", "CVAL_TOKEN", "RVALUE_TOKEN", "VTYPE_TOKEN", "ERROR_TOKEN"
};

int line_count = 1, char_count = 1;

static FILE *target_fptr = NULL;
TokenData *current_token = NULL;
TokenData *stocked_token = NULL;

char* to_lowercase(char *word) {
    // Pascal is case insensitive (program == Program == PROGRAM)
    for (char *c = word; *c != '\0'; c++)
        *c = tolower(*c);
    return word;
}

int is_keyword(char *word) {
    // Verifies if the given word is a keyword and returns the correct TokenType
    for (int i = 0; i < KEYWORD_COUNT; i++) {
        if (!strcmp(word, keywords[i]))
            return i+1;
    }
    return 0;
}

int is_special(char *word) {
    // Verifies if the given word is a special character and returns the correct TokenType
    for (int i = 0; i < SPECIAL_COUNT; i++) {
        if (!strcmp(word, specials[i]))
            return i+1+KEYWORD_COUNT;
    }
    return 0;
}

int is_number(char *word) {
    // Verifies if the given word is a number (Integer or decimal)
    int is_decimal = 0;
    int length = strlen(word);
    if (!length)
        return 0;
    for (int i = 0; i < length; i++) {
        if ((word[i] >= '0' && word[i] <= '9') || (i == 0 && (word[i] == '+' || word[i] == '-') && length > 1))
            continue;
        if (length != 1 && word[i] == '.' && !is_decimal)
            is_decimal = 1;
        else
            return 0;
    }
    return 1 + is_decimal; // 1 if integer and 2 if decimal (for now it doesn't matter)
}

int is_identifier(char *word) {
    // Verifies if a word is a valid identifier
    char str[2] = "\0";
    str[0] = word[0];
    if (!is_number(str) && !is_special(word) && !is_keyword(word))
        return 1;
    return 0;
}

TokenType find_token_type(char *word) {
    // Runs all previous tests on a given word
    if (word[0] == EOF)
        return EOF_TOKEN;
    word = to_lowercase(word);
    int is_num;
    if (is_identifier(word))
        return ID_TOKEN;
    else if ((is_num = is_number(word)) == 1)
        return INUM_TOKEN;
    else if (is_num == 2)
        return RNUM_TOKEN;
    TokenType type = is_keyword(word);
    if (type)
        return type;
    type = is_special(word);
    if (type)
        return type;
    return ERROR_TOKEN; // If all previous tests failed (identifier starting with a digit)
}

void close_target_file() {
    if (current_token != NULL) {
        free(current_token->token);
        free(current_token);
        current_token = NULL;
    }
    if (target_fptr != NULL) {
        fclose(target_fptr);
        target_fptr = NULL;
    }
}

int open_target_file(const char *path) {
    close_target_file();
    if (path == NULL) {
        printf("Error: no target source file path specified\n");
        return 0;
    }
    target_fptr = fopen(path, "r+");
    if (!target_fptr) {
        printf("Error: failed to find target source file at path \"%s\"\n", path);
        return 0;
    }
    return 1;
}

void next_token() {
    if (current_token != NULL) { // Freeing the memory allocated to the previous token (No need to go back to previous tokens ?)
        free(current_token->token);
        free(current_token);
        current_token = NULL;
    }
    if (target_fptr != NULL) {
        if (stocked_token != NULL) { // Next token has already been read in a previous call (Case of a special character)
            current_token = stocked_token;
            stocked_token = NULL;
            printf("%s -> %s\n", current_token->token, token_type_map[(current_token->type) - 1]);
            return;
        }
        // Reads the targeted file to scan for the next token
        int i = 0, n = 1, token_length = 0;
        char c = '\0', *buffer = malloc(n * BUFFER_SIZE);
        char *spec_str = NULL;
        while (c != EOF && (c = getc(target_fptr)) != EOF) {
            char_count++;
            token_length++;
            if (c == ' ' || c == '\n' || c == '\t') {
                if (i != 0) {
                    buffer[i] = '\0';
                    TokenType type = find_token_type(buffer);
                    if (type == ERROR_TOKEN) {
                        printf("Error: identifer %s starts with a digit at line %d, char %d\n", buffer, line_count, char_count-token_length);
                        return;
                    }
                    printf("%s -> %s\n", buffer, token_type_map[type - 1]);
                    current_token = malloc(sizeof(TokenData));
                    current_token->token = buffer;
                    current_token->type = type;
                    current_token->start_ln = line_count;
                    current_token->start_col = char_count-token_length;
                    if (c == '\n') {
                        char_count = 1;
                        line_count++;
                    }
                    return;
                }
                if (c == '\n') {
                    char_count = 1;
                    line_count++;
                }
                token_length = 0;
            }
            else if (c == '{') { // Case of Pascal comments
                int comment_length = 1;
                while ((c = getc(target_fptr)) != '}' && c != EOF) {
                    comment_length++;
                    if (c == '\n') {
                        comment_length = 1;
                        line_count++;
                    }
                }
                if (i != 0) {
                    buffer[i] = '\0';
                    TokenType type = find_token_type(buffer);
                    if (type == ERROR_TOKEN) {
                        printf("Error: identifer %s starts with a digit at line %d, char %d\n", buffer, line_count, char_count-token_length);
                        return;
                    }
                    printf("%s -> %s\n", buffer, token_type_map[type - 1]);
                    current_token = malloc(sizeof(TokenData));
                    current_token->token = buffer;
                    current_token->type = type;
                    current_token->start_ln = line_count;
                    current_token->start_col = char_count-token_length;
                    if (c == '}')
                        char_count += comment_length + 1;
                    return;
                }
                if (c == '}')
                    char_count += comment_length + 1;
            }
            else if (c == '\'') { // String detection
                n = 1;
                int j = 0;
                char *str_buffer = malloc(n * BUFFER_SIZE);
                while ((c = getc(target_fptr)) != '\'' && c != EOF && c != '\n') {
                    char_count++;
                    token_length++;
                    str_buffer[j++] = c;
                    if (j >= n * BUFFER_SIZE) {
                        char *new_buffer = realloc(str_buffer, ++n * BUFFER_SIZE);
                        if (new_buffer == NULL) {
                            // NULL value returned, failure to reallocate more memory for some reason
                            printf("Error: failed to reallocate more memory to the buffer\n");
                            return;
                        }
                        str_buffer = new_buffer;
                    }
                }
                str_buffer[j] = '\0';
                if (c == '\n') {
                    printf("Error: string literal exceeds line at line %d, char %d\n", line_count, char_count-token_length);
                    return;
                }
                if (c != '\'') {
                    printf("Error: unclosed string literal at line %d, char %d\n", line_count, char_count-token_length);
                    return;
                }
                stocked_token = malloc(sizeof(TokenData));
                stocked_token->token = str_buffer;
                stocked_token->type = j == 1 ? CVAL_TOKEN : SVAL_TOKEN;
                stocked_token->start_ln = line_count;
                stocked_token->start_col = char_count-token_length+i;
                if (i != 0) {
                    buffer[i] = '\0';
                    current_token = malloc(sizeof(TokenData));
                    current_token->token = buffer;
                    current_token->type = find_token_type(buffer);
                    current_token->start_ln = line_count;
                    current_token->start_col = char_count-token_length;
                }
                else {
                    current_token = stocked_token;
                    stocked_token = NULL;
                }
                printf("%s -> %s\n", current_token->token, token_type_map[current_token->type - 1]);
                return;
            }
            else if (i < n * BUFFER_SIZE) {
                // Case of composed special characters (:=, >=, <=, <>)
                spec_str = malloc(MAX_SPECIAL_SIZE+1);
                memset(spec_str, 0, MAX_SPECIAL_SIZE+1);
                int j;
                spec_str[0] = c;
                for (j = 1; j < MAX_SPECIAL_SIZE; j++) {
                    char cc = getc(target_fptr);
                    char_count++;
                    token_length++;
                    if (cc == EOF)
                        break;
                    spec_str[j] = cc;
                }
                int special = 0;
                TokenType type;
                for (int k = j-1; k >= 0; k--) { // Looks for the longest matching special character possible
                    buffer[i] = '\0';
                    if ((special = is_special(spec_str)) && (special != PERIOD_TOKEN || !is_number(buffer))) {
                        if (i != 0) {
                            type = find_token_type(buffer);
                            if (type == ERROR_TOKEN) {
                                printf("Error: identifer %s starts with a digit at line %d, char %d\n",
                                    buffer, line_count, char_count-token_length);
                                return;
                            }
                        }
                        stocked_token = malloc(sizeof(TokenData));
                        stocked_token->token = spec_str;
                        stocked_token->type = special;
                        stocked_token->start_ln = line_count;
                        stocked_token->start_col = char_count-k;
                        if (i != 0)
                            stocked_token->start_col--;
                        break;
                    }
                    if (k != 0) {
                        char_count--;
                        token_length--;
                        ungetc(spec_str[k], target_fptr); // Returns the file pointer to its previous state.
                    }
                    spec_str[k] = '\0';
                }
                if (special && (special != PERIOD_TOKEN || !is_number(buffer))) {
                    if (i != 0) {
                        current_token = malloc(sizeof(TokenData));
                        current_token->token = buffer;
                        current_token->type = type;
                        current_token->start_ln = line_count;
                        current_token->start_col = char_count-token_length;
                    }
                    else {
                        current_token = stocked_token;
                        current_token->start_col--;
                        stocked_token = NULL;
                    }
                    printf("%s -> %s\n", current_token->token, token_type_map[current_token->type - 1]);
                    return;
                }
                if (i < n * BUFFER_SIZE - 1)
                    buffer[i++] = c;
                else { // Identifier name length exceeds buffer size. (Should a limit be set ? For now I keep reallocating indefinitely)
                    printf("Warning: identifier name length exceeds buffer size at line %d, char %d\n", line_count, char_count-token_length);
                    char *new_buffer = realloc(buffer, ++n * BUFFER_SIZE);
                    if (new_buffer == NULL) {
                        // NULL value returned, failure to reallocate more memory for some reason
                        printf("Error: failed to reallocate more memory to the buffer\n");
                        return;
                    }
                    free(buffer);
                    buffer = new_buffer;
                    buffer[i++] = c;
                    buffer[i] = '\0';
                }
            }
        }
        if (spec_str == NULL) {
            spec_str = malloc(MAX_SPECIAL_SIZE+1);
        }
        memset(spec_str, 0, MAX_SPECIAL_SIZE+1);
        spec_str[0] = c;
        TokenType type = find_token_type(spec_str);
        printf("%s -> %s\n", spec_str, token_type_map[type - 1]);
        current_token = malloc(sizeof(TokenData));
        current_token->token = spec_str;
        current_token->type = type;
        current_token->start_ln = line_count;
        if (i != 0)
            char_count--;
        current_token->start_col = char_count;
    }
}

void scan_file(const char *path) {
    if (open_target_file(path)) {
        do {
            next_token();
        } while (current_token != NULL && current_token->type != EOF_TOKEN);
        close_target_file();
    }
}
