#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "symbol_types.h"

#define INITIAL_CAPACITY 2

typedef struct {
    char *identifier;
    enum SymbolTypes type;
    int declared;
    int initialised;
    int sanitised;
    int line;
    int col;
} SYMTABNODE;

typedef  SYMTABNODE        *SYMTABNODEPTR;


typedef struct {
    SYMTABNODEPTR *array;
    int in_use;
    int capacity;
} DYNAMIC_SYMTAB;

DYNAMIC_SYMTAB *symTabRec;

DYNAMIC_SYMTAB *create_dynamic_symtab();
int add_symbol(DYNAMIC_SYMTAB *array, SYMTABNODEPTR element);
int lookup_symbol(char *, DYNAMIC_SYMTAB *);
int reset_dynamic_symtab(DYNAMIC_SYMTAB *array);
void destroy_symtab(DYNAMIC_SYMTAB *);
#endif