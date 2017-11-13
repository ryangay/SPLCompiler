#include "include/optimise_tree.h"
#include "include/splio.h"
#include "include/tree_procedures.h"
#include "include/types.h"

enum OperatorType {ADD, SUBTRACT, MUL, DIV};

static void elim_variables(TERNARY_TREE *);
static TERNARY_TREE fold_constants(TERNARY_TREE, TERNARY_TREE, enum OperatorType);
static void fold_expression(TERNARY_TREE *);
static void fold_term(TERNARY_TREE *);

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