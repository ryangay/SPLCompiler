#include <stdlib.h>
#include <string.h>
#include "include/splio.h"
#include "include/symbol_table.h"
#include "include/tree_procedures.h"
#include "include/utils.h"

int *lineno;
int *colno;

TERNARY_TREE create_inode(int ival, int case_identifier, TERNARY_TREE p1,
			 TERNARY_TREE  p2, TERNARY_TREE  p3)
{
    TERNARY_TREE t;
    t = (TERNARY_TREE)malloc(sizeof(TREE_NODE));
    t->item = ival;
    t->nodeIdentifier = case_identifier;
    t->first = p1;
    t->second = p2;
    t->third = p3;
    return (t);
}

#ifdef DEBUG
void PrintTree(TERNARY_TREE t, int level)
{
    if(t == NULL) return;
	int indent_count = 0;
    for(indent_count = 0; indent_count < level; indent_count++) printf("---");
    printf("Node Id: %s", NODE_TYPE_NAMES[t->nodeIdentifier]);
    if(t->item != NOTHING)
    {
        printf(",  ");
        switch(t->nodeIdentifier)
        {
            case INT_CONST:
                printf("Integer value: %d", t->item);
                break;
            case NEG_INT_CONST:
                printf("Integer value: %d", -t->item);
                break;
            case FLOAT_CONST:
                printf("Float value: %s", symTabRec->array[t->item]->identifier);
                break;
            case NEG_FLOAT_CONST:
                printf("Float value: %s", make_float_negative(symTabRec->array[t->item]->identifier));
                break;
            case CHAR_CONST:
                printf("Character value: %c", (char)t->item);
                break;
            case ID_VAL:
                printf("Identifier value: %s", symTabRec->array[t->item]->identifier);
                break;
            case TYPE_P:
                switch(t->item)
                {
                    case CHAR_T:
                        printf("Type: Integer");
                        break;
                    case INT_T:
                        printf("Type: Real");
                        break;
                    case REAL_T:
                        printf("Type: Character");
                        break;
                }
                break;
            default:
                printf("Item value: %d", t->item);
        }
    }
    putchar('\n');
    level++;
    PrintTree(t->first, level);
    PrintTree(t->second, level);
    PrintTree(t->third, level);
    level--;
}
#endif
