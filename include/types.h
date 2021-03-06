#ifndef TREE_TYPES_H
#define TREE_TYPES_H

/* ------------- parse tree definition --------------------------- */

/* Use the preprocesser to generate enum and list of enum strings
** Adapted from https://stackoverflow.com/a/10966395 ** */
#define FOREACH_NODE_TYPE(CREATE) \
CREATE(PROGRAM) CREATE(BLOCK) CREATE(DECLARATION_BLOCK) \
CREATE(DECLARATION) CREATE(ID_LIST) CREATE(TYPE_P) \
CREATE(STATEMENT_LIST) CREATE(STATEMENT) CREATE(ASSIGNMENT) CREATE(IF_S) \
CREATE(DO_S) CREATE(WHILE_S) CREATE(FOR_S) CREATE(FOR_ASSIGN) CREATE(FOR_PROPERTIES) CREATE(LOOP_BODY) CREATE(WRITE_S) CREATE(WRITE_NEWLINE) CREATE(READ_S) \
CREATE(OUTPUT_LIST) CREATE(CONDITIONAL) CREATE(NEGATION) CREATE(LOG_AND) CREATE(LOG_OR) CREATE(COMPARISON) CREATE(COMPARATOR) CREATE(EXPRESSION) \
CREATE(TERM) CREATE(EXPR_ADD) CREATE(EXPR_MINUS) CREATE(TERM_MUL) CREATE(TERM_DIV) CREATE(VAL_IDENTIFIER) CREATE(VAL_CONSTANT) CREATE(VAL_EXPR) \
CREATE(NUMBER_CONST) CREATE(CHAR_CONST) CREATE(INT_CONST) CREATE(NEG_INT_CONST) CREATE(FLOAT_CONST) CREATE(NEG_FLOAT_CONST) CREATE(ID_VAL) \

#define CREATE_ENUM(NODE_TYPE) NODE_TYPE,
#define CREATE_STRING(NODE_TYPE) #NODE_TYPE,

enum ParseTreeNodeType {FOREACH_NODE_TYPE(CREATE_ENUM)};  

extern const char *NODE_TYPE_NAMES[];

enum CompareSymType {SYM_EQ_TO, SYM_NEQ_TO, SYM_LESS_THAN, SYM_GREATER_THAN, SYM_LESS_THAN_EQ, SYM_GREATER_THAN_EQ};

#define NOTHING        -1

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

struct treeNode {
    int  item;
    int  nodeIdentifier;
    struct treeNode *first;
    struct treeNode *second;
    struct treeNode *third;
  };

typedef  struct treeNode TREE_NODE;
typedef  TREE_NODE        *TERNARY_TREE;

#endif /* TREE_TYPES_H */