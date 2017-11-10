#include <stdlib.h>

#include "include/splio.h"
#include "include/symbol_table.h"

DYNAMIC_SYMTAB *create_dynamic_symtab()
{
    DYNAMIC_SYMTAB *new_array = (DYNAMIC_SYMTAB *)malloc(sizeof(DYNAMIC_SYMTAB));
    new_array->array = (SYMTABNODEPTR *)malloc(sizeof(SYMTABNODEPTR) * INITIAL_CAPACITY);
    new_array->capacity = INITIAL_CAPACITY;
    new_array->in_use = 0;
    return new_array;
}

int add_symbol(DYNAMIC_SYMTAB *array, SYMTABNODEPTR element)
{
    INFO("Enter add symbol procedure..\n");
    if(array->in_use == array->capacity)
    {
        INFO("Array has reached capacity (%d), doubling capacity..\nCurrent pointer: %p\n", array->capacity, array->array);
        SYMTABNODEPTR *orig = array->array;
        int new_capacity = array->capacity*2;
        array->array = realloc(array->array, sizeof(SYMTABNODEPTR) * new_capacity);
        if(array->array == NULL) 
        { 
            array->array = orig;
            return -1;
        }
        INFO("New array pointer is at %p, new capacity is %d.\n", array->array, new_capacity);
        array->capacity = new_capacity;
    }
    INFO("Adding identifier \"%s\" to symbol table..\n", element->identifier);
    int index = array->in_use;
    array->array[array->in_use++] = element;
    INFO("Added element to array: Identifier: %s\n", array->array[index]->identifier);
    return array->in_use-1;
}

int lookup_symbol(char *s, DYNAMIC_SYMTAB *symTab)
{
    int i;
    int in_use = symTab->in_use;
    SYMTABNODEPTR *array = symTab->array;

    INFO("Sym Table array at %p, Current size: %d\n", array, in_use);
    if(in_use > 0) 
    {
        INFO("Identifier of last item: %s\n", array[in_use-1]->identifier);
    }
    for(i=0; i<in_use; i++)
    {
        if(strcmp(s, array[i]->identifier) == 0)
        {
            return (i);
        }
    }
    return (-1);    
}

int reset_dynamic_symtab(DYNAMIC_SYMTAB *array)
{
    free(array->array);
    array->array = (SYMTABNODEPTR *)malloc(sizeof(SYMTABNODEPTR)*INITIAL_CAPACITY);
    array->capacity = INITIAL_CAPACITY;
    array->in_use = 0;
    if(array->array == NULL)
        return -1;
    return 0;
}

void destroy_symtab(DYNAMIC_SYMTAB *array)
{
    free(array->array);
    free(array);
}