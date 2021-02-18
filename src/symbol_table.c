#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include "symbol_table.h"

uint32_t *DEFAULT_SEED = NULL;

SymbolTable *main_table = NULL;

TokenType declaration_value_map(TokenType type) {
    if (type == INT_TOKEN)
        return INUM_TOKEN;
    if (type == INUM_TOKEN)
        return INT_TOKEN;
    if (type == REAL_TOKEN)
        return RNUM_TOKEN;
    if (type == RNUM_TOKEN)
        return REAL_TOKEN;
    if (type == CHAR_TOKEN)
        return CVAL_TOKEN;
    if (type == CVAL_TOKEN)
        return CHAR_TOKEN;
    if (type == STRING_TOKEN)
        return SVAL_TOKEN;
    if (type == SVAL_TOKEN)
        return STRING_TOKEN;
    return -1;
}

uint32_t hash(const void *key, int len, uint32_t seed) {
    // This is a copy of the excellent murmur2 hash function
    if (seed == 0) { // Define a fixed random seed for the entirety of runtime (Do not free DEFAULT_SEED until program's end)
        if (!DEFAULT_SEED) {
            srand(time(0));
            DEFAULT_SEED = malloc(sizeof(uint32_t));
            do {
                *DEFAULT_SEED = (uint32_t) (rand()*100);
            } while(*DEFAULT_SEED == 0);
        }
        seed = *DEFAULT_SEED;
    }

    const uint32_t m = 0x5bd1e995;
    const int r = 24;
    uint32_t h = seed ^ len; // Initialize the hash to a 'random' value
    const unsigned char *data = (const unsigned char *) key; // Mix 4 bytes at a time into the hash

    while (len >= 4) {
        uint32_t k = *(uint32_t*)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }

    switch(len) { // Handle the last few bytes of the input array
        case 3:
            h ^= data[2] << 16;
        case 2:
            h ^= data[1] << 8;
        case 1:
            h ^= data[0];
            h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few bytes are well-incorporated.
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h % HASH_SIZE;
}

Symbol* make_symbol(char *name, TokenType dt, TokenType tt, int line, int dim, ParamType *param_list, SymbolValue *value_list) {
    Symbol *new_symbol = malloc(sizeof(Symbol));
    new_symbol->name = malloc(strlen(name)+1);
    strcpy(new_symbol->name, name);
    new_symbol->declaration_type = dt;
    new_symbol->token_type = tt;
    new_symbol->line = line;
    new_symbol->dimension = dim;
    new_symbol->param_list = param_list;
    new_symbol->values = value_list;
    return new_symbol;
}

SymbolValue* make_symbol_value(char *token_value, TokenType target_type) {
    SymbolValue *new_value = malloc(sizeof(SymbolValue));
    if (target_type == INUM_TOKEN)
        new_value->i = atoi(token_value);
    else if (target_type == RNUM_TOKEN)
        new_value->f = atof(token_value);
    else if (target_type == CVAL_TOKEN)
        new_value->c = *token_value;
    else if (target_type == SVAL_TOKEN) {
        new_value->str = malloc(strlen(token_value)+1);
        strcpy(new_value->str, token_value);
    }
    return new_value;
}

SymbolTable* make_table(SymbolTable *previous) {
    SymbolTable *tmp = malloc(sizeof(SymbolTable));
    tmp->parent = previous;
    tmp->child = NULL;
    if (previous)
        previous->child = tmp;
    for (int i = 0; i < HASH_SIZE; i++)
        tmp->symbol_table[i] = NULL;
    tmp->symbol_count = 0;
    tmp->nesting_level = previous == NULL ? 0 : previous->nesting_level+1;
    return tmp;
}

void init_main_table() { // Don't forget to call this on start !!
    if (main_table == NULL) {
        main_table = make_table(NULL);
        SymbolValue *value_ptr = malloc(sizeof(SymbolValue));
        value_ptr->i = 1;
        symbol_insert(main_table, make_symbol("true", VAR_TOKEN, INT_TOKEN, 0, 1, NULL, value_ptr));
        value_ptr->i = 0;
        symbol_insert(main_table, make_symbol("false", VAR_TOKEN, INT_TOKEN, 0, 1, NULL, value_ptr));
    }
}

void symbol_insert(SymbolTable *table_ptr, Symbol *symbol) { // Simply inserts at list's head (Does not check for redundancy)
    int index = hash(symbol->name, strlen(symbol->name), 0);
    symbol->next = table_ptr->symbol_table[index]; // Insertion at table's head
    table_ptr->symbol_table[index] = symbol;
    table_ptr->symbol_count++;
}

Symbol* symbol_lookup(SymbolTable *table_ptr, char *name) {
    int index = hash(name, strlen(name), 0);
    Symbol *tmp = table_ptr->symbol_table[index];
    while (tmp) {
        if (strcmp(tmp->name, name) == 0)
            break;
        tmp = tmp->next;
    }
    return tmp; // Returns NULL if symbol's not found on given table
}

Symbol* symbol_deep_lookup(char *name) { // Looks up in the current and the main table before returning the result
    Symbol *tmp = symbol_lookup(current_table, name);
    if (tmp == NULL && current_table->nesting_level != main_table->nesting_level)
        tmp = symbol_lookup(main_table, name);
    return tmp;
}

void symbol_memfree(Symbol *symbol) {
    if (symbol) {
        symbol->name = NULL;
        ParamType *param = symbol->param_list;
        while (param && param->param_symbol) {
            Symbol *to_free = param->param_symbol;
            param = param->next;
            symbol_memfree(to_free);
        }
        if (symbol->param_list)
            free(symbol->param_list);
        if (symbol->values)
            free(symbol->values);
        free(symbol);
    }
}

int symbol_lookup_insert(SymbolTable *table_ptr, Symbol *symbol) {
    int index = hash(symbol->name, strlen(symbol->name), 0);
    Symbol *tmp = table_ptr->symbol_table[index], *previous = NULL;
    while (tmp) {
        if (strcmp(tmp->name, symbol->name) == 0)
            break;
        previous = tmp;
        tmp = tmp->next;
    }
    if (!tmp) { // Symbol was not found, insert it at list's head
        symbol->next = table_ptr->symbol_table[index];
        table_ptr->symbol_table[index] = symbol;
        table_ptr->symbol_count++;
        return 0;
    }
    if (symbol->declaration_type != CONST_TOKEN) {
        if (!previous) { // Symbol at list's head
            table_ptr->symbol_table[index] = symbol;
            symbol->next = tmp->next;
        }
        else { // Symbol at middle or end of list
            previous->next = symbol;
            symbol->next = tmp->next;
        }
        symbol_memfree(tmp);
    }
    return 1;
}

void symbol_delete(SymbolTable *table_ptr, char *name) {
    int index = hash(name, strlen(name), 0);
    Symbol *tmp = table_ptr->symbol_table[index], *previous = NULL;
    while (tmp) {
        if (strcmp(tmp->name, name) == 0) {
            break;
        }
        previous = tmp;
        tmp = tmp->next;
    }
    if (tmp) {
        if (!previous) { // Symbol is at list's head
            table_ptr->symbol_table[index] = tmp->next;
            if (!table_ptr->symbol_table[index])
                table_ptr->symbol_table[index] = malloc(sizeof(Symbol));
        }
        else // Symbol is in middle or end of table
            previous->next = tmp->next;
        symbol_memfree(tmp);
        table_ptr->symbol_count--;
    }
}

void clean_table(SymbolTable *table_ptr) {
    for (int i = 0; i < HASH_SIZE; i++) {
        while (table_ptr->symbol_table[i] && table_ptr->symbol_table[i]->next) {
            Symbol *to_free = table_ptr->symbol_table[i];
            table_ptr->symbol_table[i] = table_ptr->symbol_table[i]->next;
            symbol_memfree(to_free);
        }
        if (table_ptr->symbol_table[i])
            symbol_memfree(table_ptr->symbol_table[i]);
    }
    free(table_ptr);
}
