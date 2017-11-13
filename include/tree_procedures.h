#ifndef TREE_PROCEDURES_H
#define TREE_PROCEDURES_H

#include <stdio.h>

#include "symbol_table.h"
#include "types.h"

TERNARY_TREE create_inode(int ival, int case_identifier, TERNARY_TREE p1,
    TERNARY_TREE  p2, TERNARY_TREE  p3);
    
void Optimise(TERNARY_TREE*);

#ifdef DEBUG
void PrintTree(TERNARY_TREE, int);
#else
int GenerateC(TERNARY_TREE,int,FILE*);
#endif  /* DEBUG */
#endif /* TREE_PROCEDURES_H */