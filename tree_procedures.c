#include <stdlib.h>
#include <string.h>
#include "include/splio.h"
#include "include/tree_procedures.h"

int *lineno;
int *colno;

enum OperatorType {ADD, SUBTRACT, MUL, DIV};

char *RESERVED_WORDS[] = {"auto", "double", "int", "struct", "break", "else", "long", "switch", "case", "enum", "register", "typedef", "char", "extern", "return", "union", "const", "float", "short", "unsigned", "continue", "for", "signed", "void", "default", "goto", "sizeof", "volatile", "do", "if", "static", "while"};


static void sanitise_identifier(char **);
static void fold_term(TERNARY_TREE *);
static void fold_expression(TERNARY_TREE *);

static void sanitise_identifier(char **id_ptr)
{
    INFO("Sanitising identifier: %s\n", *id_ptr)
    static unsigned int gen_var_count;
    const char gen_var_prefix = 'v';
    int i;
    char *id = *id_ptr;
    int len = (int)(sizeof(RESERVED_WORDS) / sizeof(RESERVED_WORDS[0]));
    for(i=0; i < len; i++) {
        char *rsrvd = RESERVED_WORDS[i];
        size_t rsrvd_len = strlen(rsrvd);
        size_t id_len    = strlen(id);
        if(!strncmp(id, rsrvd, id_len > rsrvd_len ? id_len : rsrvd_len)) {
            char *var_name = NULL;
            do {
            free(var_name);
            size_t name_length = 
                1 + snprintf(NULL, 0, "%u", gen_var_count) + 1;
            var_name = (char *)malloc(name_length);
            snprintf(var_name, name_length, "%c%u", gen_var_prefix, gen_var_count);
            } while (lookup_symbol(var_name, symTabRec) >= 0);
            INFO("Sanitised variable name: %s\n", var_name)
            *id_ptr = var_name;
            return;
        }
    }
}

static char *make_float_negative(char *float_lit)
{
    INFO("Convering %s to a negative REAL", float_lit)
    size_t float_literal_len = strlen(float_lit);
    char *literal_with_neg = (char *)malloc(float_literal_len + 1);
    literal_with_neg[0] = '-';

    strcat(literal_with_neg, float_lit);

    return literal_with_neg;
}

static char *get_formatter(enum SymbolTypes type)
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

static void elim_variables(TERNARY_TREE  *t) {
    /* Define a structure to hold the current symbol data */
    typedef struct {
        SYMTABNODEPTR node;
        union {
            int integer;
            char character;
            double real;
        } val;
        int has_const_val;
    } SYMTABNODEDATA;

    SYMTABNODEDATA **symtabnode_data = malloc(sizeof(SYMTABNODEDATA *)*symTabRec->in_use);
    int node_data_num;
    for(node_data_num = 0; node_data_num < symTabRec->in_use; node_data_num++) {
        SYMTABNODEDATA *new = malloc(sizeof(SYMTABNODEDATA));
        new->node = symTabRec->array[node_data_num];
        new->has_const_val = 0;
        symtabnode_data[node_data_num] = new;
    }
}

static TERNARY_TREE fold_constants(TERNARY_TREE left, TERNARY_TREE right, enum OperatorType op) {

    int left_is_number = left->nodeIdentifier == NUMBER_CONST;
    int right_is_number = right->nodeIdentifier == NUMBER_CONST;
    
    INFO("Optimisation: Folding expression: Left is%sa number, Right is%sa number\n", left_is_number ? " " : " not ", right_is_number ? " " : " not ")

    if( (!left_is_number && left->nodeIdentifier != CHAR_CONST)
        || (!right_is_number && right->nodeIdentifier != CHAR_CONST) ) return NULL;

    INFO("Optimisation: Folding expression: Types are correct\n")

    /*We can fold chars and integers. However, we will leave the C compiler to interpret the given floating-point values */
    if( left_is_number
        && ( (left->first->nodeIdentifier == FLOAT_CONST) || (left->first->nodeIdentifier == NEG_FLOAT_CONST) ) ) {
        return NULL;
    }
    if( right_is_number 
        && ( (right->first->nodeIdentifier == FLOAT_CONST) || (right->first->nodeIdentifier == NEG_FLOAT_CONST) ) ) {
        return NULL;
    }
    /* We have integers so we are good to continue */
    TERNARY_TREE number_left = left_is_number ? left->first : NULL;
    TERNARY_TREE number_right = right_is_number ? right->first : NULL;

    int is_negative_1 = left_is_number ? ((number_left->nodeIdentifier == NEG_INT_CONST) ? 1 : 0) : 0;
    int is_negative_2 = right_is_number ? ((number_right->nodeIdentifier == NEG_INT_CONST) ? 1 : 0) : 0;
    
    INFO("Optimisation: Folding expression: Left is%snegative , Right is%snegative\n", is_negative_1 ? " " : " not ", is_negative_2 ? " " : " not ")

    int val_1;
    int val_2;

    if(left_is_number) {
        val_1 = is_negative_1 ? 0-number_left->item : number_left->item;
    }
    else val_1 = left->item;

    if(right_is_number) {
        val_2 = is_negative_2 ? 0-number_right->item : number_right->item;
    }
    else val_2 = right->item;

    int val;
    
    switch(op){
        case ADD:
            val = val_1 + val_2;
            INFO("Optimisation: Folded expression: %d + %d = %d\n", val_1, val_2, val)
            break;
        case SUBTRACT:
            val = val_1 - val_2;
            INFO("Optimisation: Folded expression: %d - %d = %d\n", val_1, val_2, val)
            break;
        case MUL:
            val = val_1 * val_2;
            INFO("Optimisation: Folded expression: %d * %d = %d\n", val_1, val_2, val)
            break;
        case DIV:
            if(val_2 == 0) {
                return NULL;
            }
            else {
                val = val_1 / val_2;
                INFO("Optimisation: Folded expression: %d / %d = %d\n", val_1, val_2, val)
            }
    }

    int val_is_negative = val < 0;

    TERNARY_TREE constant_bit = !(left_is_number && right_is_number)
                              ? create_inode(val, CHAR_CONST, NULL, NULL, NULL)
                              : create_inode(NOTHING, NUMBER_CONST,
                                    create_inode(val_is_negative ? 0-val : val, val_is_negative ? NEG_INT_CONST : INT_CONST, NULL, NULL, NULL),
                                    NULL, NULL);


    INFO("Optimisation: Folding expression: Restructuring tree\n")
    
    return create_inode(NOTHING, TERM,
            create_inode(NOTHING, VAL_CONSTANT, constant_bit, NULL, NULL),
            NULL, NULL);

}

static void fold_expression(TERNARY_TREE *t)
{
    if( (t == NULL) || (*t == NULL) ) {
        /* This term does not exist */
        return;
    }
    TERNARY_TREE this_node = *t;
    /* Always try to fold the first term */
    fold_term(&(this_node->first));
    if(this_node->first->nodeIdentifier != TERM || this_node->first->first->nodeIdentifier != VAL_CONSTANT) {
        /* If the first branch could not be folded, then we cannot fold the expression */
        return;
    }
    if( this_node->nodeIdentifier == EXPRESSION ) {
        /* Can't do anythong more here */
        return;
    }

    if(this_node->second->nodeIdentifier != EXPRESSION) {
        /* Try to fold the constants within this expression */
        fold_expression(&(this_node->second));
        if(this_node->second->nodeIdentifier != EXPRESSION 
            || this_node->second->first->nodeIdentifier != TERM 
            || this_node->second->first->first->nodeIdentifier != VAL_CONSTANT) return;
    }
    
    /* Start trying to fold the constants within the expression */
    TERNARY_TREE sub_expr_2  = this_node->second; /*  EXPRESSION->EXPRESSION */
    TERNARY_TREE term_1      = this_node->first; /*               EXPRESSION->TERM */
    TERNARY_TREE term_2      = sub_expr_2->first; /*  EXPRESSION->EXPRESSION->TERM */
    TERNARY_TREE val_const_1 = term_1->first; /*                  EXPRESSION->TERM->VAL_CONSTANT */
    TERNARY_TREE val_const_2 = term_2->first; /*      EXPRESSION->EXPRESSION->TERM->VAL_CONSTANT */
    TERNARY_TREE const_1     = val_const_1->first; /*             EXPRESSION->TERM->VAL_CONSTANT->(SOME CONSTANT) */
    TERNARY_TREE const_2     = val_const_2->first; /* EXPRESSION->EXPRESSION->TERM->VAL_CONSTANT->(SOME CONSTANT) */

    int const_1_is_number = const_1->nodeIdentifier == NUMBER_CONST;
    int const_2_is_number = const_2->nodeIdentifier == NUMBER_CONST;
    TERNARY_TREE number_const_1 = const_1_is_number ? const_1->first : NULL;
    TERNARY_TREE number_const_2 = const_2_is_number ? const_2->first : NULL;

    enum OperatorType op;

    if(this_node->nodeIdentifier == EXPR_ADD) {
        op = ADD;
    }
    else if(this_node->nodeIdentifier == EXPR_MINUS) {
        op = SUBTRACT;
    }
    else return;

    INFO("Optimisation: About to fold constants within expression\n")
    TERNARY_TREE folded_term = fold_constants(const_1, const_2, op);

    if(!folded_term) return;

    *t = create_inode(NOTHING, EXPRESSION, folded_term, NULL, NULL);


    /* Clean up */
    free(sub_expr_2);
    free(term_1);
    free(term_2);
    free(val_const_1);
    free(val_const_2);
    free(const_1);
    free(const_2);
    if(const_1_is_number) free(number_const_1);
    if(const_2_is_number) free(number_const_2);
}

static void fold_term(TERNARY_TREE *t)
{
    if( (t == NULL) || (*t == NULL) ) {
        /* This term does not exist */
        return;
    }
    TERNARY_TREE this_node = *t;
    if( (this_node->first != NULL)
        && (this_node->first->nodeIdentifier == VAL_EXPR) ) {
            /* This term contains an expression within a value node, which could be folded.. 
            /* this_node->VAL_EXPR->(SOME EXPRESSION TYPE) */
            fold_expression(&(this_node->first->first));
    }
    if(this_node->nodeIdentifier == TERM || this_node->first->nodeIdentifier != VAL_CONSTANT) {
        /* This term is already as good as it's going to get */
        return;
    }
    if(this_node->second->nodeIdentifier != TERM) {
        /* If the second term is a multiplication or division, try to fold that first */
        fold_term(&(this_node->second));

    }
    if(this_node->second->first->nodeIdentifier == VAL_EXPR) {
        fold_expression(&(this_node->second));
    }
    if( (this_node->second->nodeIdentifier != TERM)
        && (this_node->second->first->nodeIdentifier != VAL_CONSTANT) ) {
            /* ¯\_(ツ)_/¯ We tried */
            return;
        }   

    /* We can now try to fold the constants */
    TERNARY_TREE sub_term_2  = this_node->second; /*  TERM->TERM */
    TERNARY_TREE val_const_1 = this_node->first; /*         TERM->VAL_CONSTANT */
    TERNARY_TREE val_const_2 = sub_term_2->first; /*  TERM->TERM->VAL_CONSTANT */
    TERNARY_TREE const_1     = val_const_1->first; /*       TERM->VAL_CONSTANT->(SOME CONSTANT) */
    TERNARY_TREE const_2     = val_const_2->first; /* TERM->TERM->VAL_CONSTANT->(SOME CONSTANT) */

    int const_1_is_number = const_1->nodeIdentifier == NUMBER_CONST;
    int const_2_is_number = const_2->nodeIdentifier == NUMBER_CONST;

    /*We can fold chars and integers. However, we will leave the C compiler to interpret the given floating-point values */
    if( const_1_is_number
        && (const_1->first->nodeIdentifier == FLOAT_CONST) || (const_1->first->nodeIdentifier == NEG_FLOAT_CONST) ) {
        return;
        if( const_2_is_number 
            && (const_2->first->nodeIdentifier == FLOAT_CONST) || (const_2->first->nodeIdentifier == NEG_FLOAT_CONST) ) {
                return;
        }
    }
    /* We have integers so we are good to continue */
    TERNARY_TREE number_const_1 = const_1_is_number ? const_1->first : NULL;
    TERNARY_TREE number_const_2 = const_2_is_number ? const_2->first : NULL;

    int is_negative_1 = const_1_is_number ? ((const_1->nodeIdentifier == NEG_INT_CONST) ? 1 : 0) : 0;
    int is_negative_2 = const_1_is_number ? ((const_2->nodeIdentifier == NEG_INT_CONST) ? 1 : 0) : 0;    

    enum OperatorType op;
    
        if(this_node->nodeIdentifier == TERM_MUL) {
            op = MUL;
        }
        else if(this_node->nodeIdentifier == TERM_DIV) {
            op = DIV;
        }
        else return;

    TERNARY_TREE folded_term = fold_constants(const_1, const_2, op);

    if(!folded_term) {
        return;
    }
    *t = folded_term;

    /* Clean up */
    free(sub_term_2);
    free(val_const_1);
    free(val_const_2);
    free(const_1);
    free(const_2);
    if(const_1_is_number) free(number_const_1);
    if(const_2_is_number) free(number_const_2);
}


void Optimise(TERNARY_TREE *t)
{
    /* 
    /* What can we optimise?
    /* The simplest optimisation we can perform is known as "constant folding",
    /* where we evaluate or term an expression in which all of the value are already known
    */
    if( (t == NULL) || (*t == NULL) ) {
        return;
    }
    TERNARY_TREE this_node = *t;

    Optimise(&(this_node->first));
    Optimise(&(this_node->second));
    Optimise(&(this_node->third));
    /*INFO("Optimisation: Current node pointer is %p, identifier is %d", this_node, this_node->nodeIdentifier)
    INFO("Optimisation: Current node is %s\n", NODE_TYPE_NAMES[this_node->nodeIdentifier]);*/
    switch(this_node->nodeIdentifier)
    {
        case BLOCK:
            if(!this_node->first && this_node->second) {
                /* Move the code block to the 1st branch */
                this_node->first = this_node->second;
                this_node->second = NULL;
            }
            break;
        case DECLARATION_BLOCK:
            /* If there are no declarations, remove the block */
            if(this_node->first == NULL && this_node->second == NULL) {
                *t = NULL;
            }
            break;
        case DECLARATION:
            break;
        case ID_LIST:
            break;
        case TYPE_P:
            break;
        case STATEMENT_LIST:
            break;
        case STATEMENT:
            break;
        case ASSIGNMENT:
            break;
        case IF_S:
            break;
        case DO_S:
            break;
        case WHILE_S:
            break;
        case FOR_S:
            break;
        case FOR_ASSIGN:
            break;
        case FOR_PROPERTIES:
            break;
        case LOOP_BODY:
            break;
        case WRITE_S:
            break;
        case WRITE_NEWLINE:
            break;
        case READ_S:
            break;
        case OUTPUT_LIST:
            break;
        case CONDITIONAL:
            break;
        case NEGATION:
            break;
        case LOG_AND:
            break;
        case LOG_OR:
            break;
        case COMPARISON:
            break;
        case COMPARATOR:
            break;
        case EXPRESSION:
            break;
        case EXPR_ADD:
            INFO("Attempting to fold expression..\n")
            fold_expression(t);
            break;
        case EXPR_MINUS:
            INFO("Attempting to fold expression..\n")
            fold_expression(t);
            break;
        case TERM:
            break;
        case TERM_MUL:
            INFO("Attempting to fold term..\n")
            fold_term(t);
            break;
        case TERM_DIV:
            INFO("Attempting to fold term..\n")
            fold_term(t);
            break;
        case VAL_IDENTIFIER:
            break;
        case VAL_CONSTANT:
            break;
        case VAL_EXPR:
            break;
        case NUMBER_CONST:
            break;
        case CHAR_CONST:
            break;
        case INT_CONST:
            break;
        case NEG_INT_CONST:
            break;
        case FLOAT_CONST:
            break;
        case NEG_FLOAT_CONST:
            break;
        case ID_VAL:
            break;
    }
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
#else

int GenerateC(TERNARY_TREE t, int level, FILE* output)
{
    static int   declared_sym_only = FALSE;
    static int   currType;
    static int   lastType;
    static int   insideExpr;
    static char *buffer;
    static char *fmt_buffer = NULL;
    static int   fmt_buffer_length;
           char *indent = NULL;
           int indent_count = 0;
    if(level > 0) {
        indent = malloc(level*4+1);
        indent[level*4] = '\0';
    }
#define PRINTCODE(s) fprintf(output, "%s", s);
#define BUFFERCODE(s) INFO("Adjusting buffer size from %zd to %zd\n", strlen(s), strlen(s)+strlen(buffer))  \
                      buffer = (char *)realloc(buffer, strlen(s) + strlen(buffer) + 1); \
                      INFO("Buffer adjusted.\n") strncat(buffer, s, strlen(s));
#define BUFFERRESET buffer = calloc(1, sizeof(char));
#define PRINTBUFFER PRINTCODE(buffer); BUFFERRESET
#define BUFFER_FMT_STRING(fmt_string, ...) fmt_buffer_length = snprintf(NULL, 0, fmt_string, __VA_ARGS__) + 1; INFO("Fmt string length: %d\n", fmt_buffer_length) \
                                           fmt_buffer = (char *)malloc(fmt_buffer_length); snprintf(fmt_buffer, fmt_buffer_length, fmt_string, __VA_ARGS__); \
                                           INFO("Buffering string: %s\n", fmt_buffer) \
                                           BUFFERCODE(fmt_buffer) \
                                           INFO("String buffered\n") free(fmt_buffer); fmt_buffer_length = 0;
#define PRINTLINE fprintf(output, "\n%s", indent);
#define CALLTREENODE(node, level, output) if(GenerateC(node, level, output) < 0) return -1;
    for(indent_count = 0; (level > 0) && (indent_count < (level*4)); indent_count++) {
        indent[indent_count] = ' ';
    }
    static DYNAMIC_SYMTAB *tempSymTabRec;
    if(tempSymTabRec == NULL)
        tempSymTabRec = create_dynamic_symtab();

    if(t == NULL) return 1;
    switch(t->nodeIdentifier)
    {
        case PROGRAM:
        {
            int retVal = 0;
            BUFFERRESET
            GenerateC(t->first, level, output);
            SYMTABNODEPTR prog_id_node = tempSymTabRec->array[0];
            char *prog_name = prog_id_node->identifier;
            BUFFERRESET
            BUFFER_FMT_STRING("#include <stdio.h>\n\nvoid %s(void);\n\nint main(void) { %s(); return 0; }\n\nvoid %s", prog_name, prog_name, prog_name);
            PRINTBUFFER
            prog_id_node->declared = TRUE;
            prog_id_node->type = PROGRAM;
            reset_dynamic_symtab(tempSymTabRec);
            PRINTCODE("(void)\n{")
            level++;
            BUFFERRESET
            if(GenerateC(t->second, level, output) < 0) {
                retVal = -1;
                goto clean_up;
            }
            level--;
            PRINTCODE("\n}\n")
            INFO("Cleaning up..\n");
            clean_up:
                destroy_symtab(tempSymTabRec);
                if(indent != NULL)
                    free(indent);
                if(buffer != NULL)
                    free(buffer);
            return retVal;
        }
        case BLOCK:
            BUFFERRESET
            BUFFERRESET
            CALLTREENODE(t->first, level, output);
            CALLTREENODE(t->second, level, output);
            return 0;
        case DECLARATION_BLOCK:
            CALLTREENODE(t->first, level, output);
            reset_dynamic_symtab(tempSymTabRec);
            CALLTREENODE(t->second, level, output);
            declared_sym_only = TRUE;
            return 0;
        case DECLARATION:
		{
            PRINTLINE
            CALLTREENODE(t->second, level, output);
            PRINTCODE(" ");
            CALLTREENODE(t->first, level, output);
            PRINTBUFFER
            BUFFERRESET
            PRINTCODE(";")
			int sym_count = 0;
            for(sym_count = 0; sym_count < tempSymTabRec->in_use; sym_count++)
            {
                SYMTABNODEPTR current_sym = tempSymTabRec->array[sym_count];
                current_sym->type = currType;
                if(current_sym->declared) {
                    ERROR(*lineno, *colno, "Variable with identifier \"%s\" has already been declared.", current_sym->identifier)
                    return -1;
                }
                current_sym->declared = TRUE;
            }
            return 0;
		}
        case ID_LIST:
            CALLTREENODE(t->first, level, output);
            if(t->second != NULL)
            {
                BUFFERCODE(", ")
                CALLTREENODE(t->second, level, output);
            }
            return 0;
        case TYPE_P:
            currType = t->item;
            switch(t->item)
            {
                case CHAR_T:
                    PRINTCODE("char")
                    return 0;
                case INT_T:
                    PRINTCODE("int")
                    return 0;
                case REAL_T:
                    PRINTCODE("double")
                    return 0;
            }
            return 1;
        case STATEMENT_LIST:
            CALLTREENODE(t->first, level, output);
            CALLTREENODE(t->second, level, output);
            return 0;
        case STATEMENT:
            PRINTLINE
            CALLTREENODE(t->first, level, output);
            return 0;
        case ASSIGNMENT:
        {
            SYMTABNODEPTR currSym;
            BUFFERRESET
            CALLTREENODE(t->second, level, output);
            currSym = tempSymTabRec->array[0];
            BUFFERCODE(" = ")
            reset_dynamic_symtab(tempSymTabRec);
            CALLTREENODE(t->first, level, output);
            if(currSym && (currSym->type < lastType) ) {
                ERROR(0,0,"Invalid assignment: \"%s\" does not have the correct type.\n", currSym->identifier)
                return -1;
            }
            PRINTBUFFER
            PRINTCODE(";")
            currSym->initialised = 1;
            return 0;
        }
        case IF_S:
            PRINTCODE("if(")
            CALLTREENODE(t->first, level, output);
            PRINTCODE(")"); 
            PRINTLINE 
            PRINTCODE("{");
            CALLTREENODE(t->second, ++level, output); level--;
            PRINTLINE
            PRINTCODE("}")
            if(t->third != NULL)
            {
                PRINTLINE
                PRINTCODE("else"); 
                PRINTLINE
                PRINTCODE("{");
                CALLTREENODE(t->third, ++level, output); level--;
                PRINTLINE
                PRINTCODE("}")
            }
            return 0;
        case DO_S:
            PRINTCODE("do")
            CALLTREENODE(t->first, level, output);
            PRINTCODE(" while( ")
            CALLTREENODE(t->second, level, output);
            PRINTCODE(" );")
            return 0;
        case WHILE_S:
            PRINTCODE("while ( ")
            CALLTREENODE(t->first, level, output);
            PRINTCODE(" )")
            CALLTREENODE(t->second, level, output);
            return 0;
        case FOR_S:
            PRINTCODE("for( ");
            CALLTREENODE(t->first, level, output);
            PRINTCODE("; ")
            CALLTREENODE(t->second, level, output);
            PRINTCODE(" )");
            CALLTREENODE(t->third, level, output); 
            return 0;
        case FOR_ASSIGN:
            reset_dynamic_symtab(tempSymTabRec);
            BUFFERRESET
            CALLTREENODE(t->first, level, output);
            PRINTCODE(" /* identifier */ ");
            BUFFERCODE(" = ");
            CALLTREENODE(t->second, level, output);
            PRINTBUFFER
            PRINTCODE(" /* value */ ");
            tempSymTabRec->array[0]->initialised = 1;
            return 0;
        case FOR_PROPERTIES:
        {
            SYMTABNODEPTR for_iter = tempSymTabRec->array[0];
            char *loop_ident = for_iter->identifier;
            if(for_iter->type == REAL_T) {
                WARNING(*lineno, 0, "Iterator \"%s\" defined as type REAL may cause the FOR loop to run perpetually.\n", loop_ident)
            }
            BUFFERRESET
            /* Decide what the condition sign should be */
            TERNARY_TREE curr_by_tree = t->first;
            if(curr_by_tree->nodeIdentifier == EXPRESSION) {
                /* If there is no +/- operators within the expression */
                INFO("FOR loop: Iterator is EXPRESSION\n")
                curr_by_tree = curr_by_tree->first;
                if(curr_by_tree->nodeIdentifier == TERM) {
                    /* If there is no * /(divide) operators within the term   */
                    INFO("FOR loop: Iterator is TERM\n")
                    curr_by_tree = curr_by_tree->first;
                    if(curr_by_tree->nodeIdentifier == VAL_CONSTANT) {
                        /* If the "by" clause is a constant value
                        /* This makes it easier to decide what sign we should use in the condition */
                        INFO("FOR loop: Iterator is VAL_CONSTANT\n")
                        char *sign = malloc(2);
                        curr_by_tree = curr_by_tree->first;
                        if(curr_by_tree->nodeIdentifier == CHAR_CONST) {
                            /* A char is an unsigned integer constant therefore it must always be positive */
                            sign = "<=";
                        }
                        else {
                            int curr_by_tree_node = curr_by_tree->first->nodeIdentifier;
                            if(curr_by_tree_node == INT_CONST || curr_by_tree_node == FLOAT_CONST) {
                                sign = "<=";
                            }
                            else if(curr_by_tree_node == NEG_INT_CONST || 
                                    curr_by_tree_node == NEG_FLOAT_CONST) {
                                        sign = ">=";
                                    }
                            else {
                                WARNING(*lineno, *colno, "Error parsing value of BY clause of FOR loop, reverting to default \"!=\" operator.")
                                sign = "!=";
                            }
                        }
                        BUFFERCODE(loop_ident) BUFFER_FMT_STRING(" %s ", sign)
                        TREE_INFO("Buffered for-loop identifier..\n")
                        CALLTREENODE(t->second, level, output);
                    }
                }
                else {
                    BUFFERCODE("(")
                    CALLTREENODE(t->first, level, output)
                    BUFFER_FMT_STRING(" > 0 ? %s-(", loop_ident)
                    CALLTREENODE(t->second, level, output)
                    BUFFERCODE(") : (")
                    CALLTREENODE(t->second, level, output)
                    BUFFER_FMT_STRING(")-%s ) <= 0", loop_ident)
                }
            }
            else {
                BUFFERCODE("(")
                CALLTREENODE(t->first, level, output)
                BUFFER_FMT_STRING(" > 0 ? %s-(", loop_ident)
                CALLTREENODE(t->second, level, output)
                BUFFERCODE(") : (")
                CALLTREENODE(t->second, level, output)
                BUFFER_FMT_STRING(")-%s ) <= 0", loop_ident)
            }
            TREE_INFO("Generated loop condition..\n")
            BUFFER_FMT_STRING("; %s = %s+(", loop_ident, loop_ident)
            /*INFO("Start generating iterator..\n")*/
            CALLTREENODE(t->first, level, output);
            TREE_INFO("Generated iterator..\n")
            BUFFERCODE(")")
            TREE_INFO("Completed for-loop declaration\n")
            PRINTBUFFER
            BUFFERRESET
            return 0;
        }
        case LOOP_BODY:
            PRINTLINE
            PRINTCODE("{")
            CALLTREENODE(t->first, ++level, output); level--;
            PRINTLINE
            PRINTCODE("}")
            return 0;
        case WRITE_S:
		{
            reset_dynamic_symtab(tempSymTabRec);
            BUFFERRESET
            CALLTREENODE(t->first, level, output);
            INFO("Number of outputs: %d", tempSymTabRec->in_use)
            int sym_count;
            for(sym_count = 0; sym_count < tempSymTabRec->in_use; sym_count++) {
                SYMTABNODEPTR curr_sym = tempSymTabRec->array[sym_count];
                if(curr_sym->identifier && !curr_sym->initialised) {
                    ERROR(*lineno, *colno, "Attempt to WRITE uninitialised variable \"%s\"\n", curr_sym->identifier)
                    return -1;
                }
            }
            PRINTCODE("printf(\"")
            for(sym_count = 0; sym_count < tempSymTabRec->in_use; sym_count++)
            {
                PRINTCODE(get_formatter(tempSymTabRec->array[sym_count]->type))
            }
            PRINTCODE("\", ")
            PRINTBUFFER
            PRINTCODE(")")
            PRINTCODE(";")
            BUFFERRESET
            return 0;
		}
        case WRITE_NEWLINE:
            PRINTCODE("putchar('\\n');")
            return 0;
        case READ_S:
        {
            reset_dynamic_symtab(tempSymTabRec);
            enum SymbolTypes read_type = symTabRec->array[t->first->item]->type;
#ifdef DEBUG
            BUFFER_FMT_STRING("/* Type is %d */", read_type) PRINTLINE
#endif
            PRINTCODE("scanf(\"")
            BUFFERRESET
            BUFFER_FMT_STRING("%s%s\", &", read_type == CHAR_T ? " " : "", get_formatter(read_type))
            CALLTREENODE(t->first, level, output); 
            PRINTBUFFER
            PRINTCODE(")")
            PRINTCODE(";")
            tempSymTabRec->array[0]->initialised = TRUE;
            return 0;
        }
        case OUTPUT_LIST:
        {

            currType = 0;
            CALLTREENODE(t->first, level, output);
            SYMTABNODEPTR placeholder = (SYMTABNODEPTR)malloc(sizeof(SYMTABNODE));
            placeholder->type = currType;
            placeholder->identifier = NULL;
            /* Because symbols are automatically added to the symbol stack, unless within an expression */
            if(t->first->nodeIdentifier != VAL_IDENTIFIER)
                add_symbol(tempSymTabRec, placeholder);
            if(t->second != NULL)
            {
                BUFFERCODE(", ")
                CALLTREENODE(t->second, level, output);
            }
            return 0;
        }
        case CONDITIONAL:
            CALLTREENODE(t->first, level, output);
            return 0;
        case NEGATION:
            PRINTCODE("!( ")
            CALLTREENODE(t->first, level, output);
            PRINTCODE(" )")
            return 0;
        case LOG_AND:
            PRINTCODE("( ")
            CALLTREENODE(t->first, level, output);
            PRINTCODE(" && ")
            CALLTREENODE(t->second, level, output);
            PRINTCODE(" )")
            return 0;
        case LOG_OR:
            PRINTCODE("( ")
            CALLTREENODE(t->first, level, output);
            PRINTCODE(" || ")
            CALLTREENODE(t->second, level, output);
            PRINTCODE(" )")
            return 0;
        case COMPARISON:
            BUFFERRESET
            CALLTREENODE(t->first, level, output); BUFFERCODE(" ");
            CALLTREENODE(t->second, level, output); BUFFERCODE(" ");
            CALLTREENODE(t->third, level, output);
            PRINTBUFFER
            BUFFERRESET
            return 0;
        case COMPARATOR:
            switch(t->item)
            {
                case SYM_EQ_TO:
                    BUFFERCODE("==");
                    break;
                case SYM_NEQ_TO:
                    BUFFERCODE("!=");
                    break;
                case SYM_LESS_THAN:
                    BUFFERCODE("<");
                    break;
                case SYM_GREATER_THAN:
                    BUFFERCODE(">");
                    break;
                case SYM_LESS_THAN_EQ:
                    BUFFERCODE("<=");
                    break;
                case SYM_GREATER_THAN_EQ:
                    BUFFERCODE(">=");
                    break;
            }
            return 0;
        case EXPRESSION:
            {
                int notAlreadyInside = insideExpr == FALSE;
                TREE_INFO("Entered expression%s..current buffer is \n%s\n", notAlreadyInside ? " at top of expr tree" : "", buffer)
                if(notAlreadyInside) insideExpr == TRUE;
                CALLTREENODE(t->first, level, output);
                if(notAlreadyInside) insideExpr == FALSE;
            }
            return 0;
        case EXPR_ADD:
            CALLTREENODE(t->first, level, output);
            BUFFERCODE(" + ")
            CALLTREENODE(t->second, level, output);
            return 0;
        case EXPR_MINUS:
            CALLTREENODE(t->first, level, output);
            BUFFERCODE(" - ")
            CALLTREENODE(t->second, level, output);
            return 0;
        case TERM:
            TREE_INFO("Inside term..\n")
            CALLTREENODE(t->first, level, output);
            return 0;
        case TERM_MUL:
            CALLTREENODE(t->first, level, output);
            BUFFERCODE(" * ")
            CALLTREENODE(t->second, level, output);
            return 0;
        case TERM_DIV:
            CALLTREENODE(t->first, level, output);
            BUFFERCODE(" / ")
            CALLTREENODE(t->second, level, output);
            return 0;
        case VAL_IDENTIFIER:
            CALLTREENODE(t->first, level, output);
            return 0;
        case VAL_CONSTANT:
            TREE_INFO("Found a constant..\n")
            CALLTREENODE(t->first, level, output);
            return 0;
        case VAL_EXPR:
            CALLTREENODE(t->first, level, output);
            return 0;
        case NUMBER_CONST:
            TREE_INFO("Constant is a number..\n")
            CALLTREENODE(t->first, level, output);
            return 0;
        case CHAR_CONST:
            BUFFER_FMT_STRING("'%c'", (char)t->item);
            if(currType < CHAR_T) currType = CHAR_T;
            lastType = CHAR_T;
            return 0;
        case INT_CONST:
            BUFFER_FMT_STRING("%d", t->item);
            if(currType < INT_T) currType = INT_T;
            lastType = INT_T;
            return 0;
        case NEG_INT_CONST:
            TREE_INFO("Hit negative integer literal: %d\n", t->item)
            BUFFER_FMT_STRING("%d", -t->item);
            /*TREE_INFO("Literal has been buffered..\n")*/
            if(currType < INT_T) currType = INT_T;
            lastType = INT_T;
            return 0;
        case FLOAT_CONST:
            BUFFERCODE(symTabRec->array[t->item]->identifier);
            if(currType < REAL_T) currType = REAL_T;
            lastType = REAL_T;
            return 0;
        case NEG_FLOAT_CONST:
            BUFFERCODE(make_float_negative(symTabRec->array[t->item]->identifier))
            if(currType < REAL_T) currType = REAL_T;
            lastType = REAL_T;
            return 0;
        case ID_VAL:
        {
            SYMTABNODEPTR sym_ptr = symTabRec->array[t->item];
            if(!declared_sym_only && !sym_ptr->sanitised) {
                sanitise_identifier(&(sym_ptr->identifier));
            }
            if(declared_sym_only && !sym_ptr->declared) {
                ERROR(*lineno, *colno, "Unknown identifier \"%s\"\n", sym_ptr->identifier)
                add_symbol(tempSymTabRec, sym_ptr);
                return -1;
            }
            BUFFERCODE(sym_ptr->identifier);
            if(insideExpr)
            {
                if(currType < sym_ptr->type) currType = sym_ptr->type;
            }
            else add_symbol(tempSymTabRec, sym_ptr);
            return 0;
        }
    }
    if(indent != NULL) 
        free(indent);
}

#endif
