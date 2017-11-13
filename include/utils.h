#ifndef UTILS_H
#define UTILS_H

#include "symbol_types.h"

#ifndef DEBUG
extern int *lineno;
extern int *colno;
#endif

char *make_float_negative(char *);
char *get_formatter(enum SymbolTypes type);

#endif