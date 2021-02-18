#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "scanner.h"

#define HASH_SIZE 1049

typedef struct _SymbolValue {
    union Value {
        int i;
        float f;
        char c;
        char *str;
    };
} SymbolValue;

typedef struct _ParamType {
    struct _ParamType *next;
    struct _Symbol *param_symbol;
} ParamType;

typedef struct _Symbol {
    struct _Symbol *next;

    TokenType declaration_type;
    TokenType token_type;
    int line;
    int dimension; // string length, number length, array size, function/procedure params count ...
    char *name;

    ParamType *param_list; // Case of a function/procedure
    SymbolValue *values;
} Symbol;

typedef struct _SymbolTable {
    struct _SymbolTable *parent;
    struct _SymbolTable *child;
    int nesting_level;
    int symbol_count;
    Symbol *symbol_table[HASH_SIZE];
} SymbolTable;

SymbolTable *main_table;
SymbolTable *current_table;

TokenType declaration_value_map(TokenType);
Symbol* make_symbol(char *name, TokenType dt, TokenType tt, int line, int dim, ParamType *param_list, SymbolValue *value_list);
SymbolValue* make_symbol_value(char *, TokenType);
SymbolTable* make_table(SymbolTable *);
void init_main_table();
void symbol_insert(SymbolTable *, Symbol *);
Symbol* symbol_lookup(SymbolTable *, char *);
Symbol* symbol_deep_lookup(char *);
void symbol_memfree(Symbol *);
int symbol_lookup_insert(SymbolTable *, Symbol *);
void symbol_delete(SymbolTable *, char *);
void clean_table(SymbolTable *);

#endif
