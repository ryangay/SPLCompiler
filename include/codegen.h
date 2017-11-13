#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "types.h"

#ifndef DEBUG
int GenerateC(TERNARY_TREE, int, FILE *);
#endif

#endif