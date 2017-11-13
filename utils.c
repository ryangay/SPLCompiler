#include <stdlib.h>
#include <string.h>
#include "include/utils.h"

char *make_float_negative(char *float_lit)
{
    INFO("Convering %s to a negative REAL", float_lit)
    size_t float_literal_len = strlen(float_lit);
    char *literal_with_neg = (char *)malloc(float_literal_len + 1);
    literal_with_neg[0] = '-';

    strcat(literal_with_neg, float_lit);

    return literal_with_neg;
}

char *get_formatter(enum SymbolTypes type)
{
    switch(type)
    {
        case CHAR_T:
            return "%c";
        case INT_T:
            return "%d";
        case REAL_T:
            return "%lg";
        default:
            return "";
    }
}