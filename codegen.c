#ifndef DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/codegen.h"
#include "include/symbol_table.h"
#include "include/types.h"
#include "include/utils.h"

static void sanitise_identifier(char **);


char *RESERVED_WORDS[] = {"auto", "double", "int", "struct", "break", "else", "long", "switch", "case", "enum", "register", "typedef", "char", "extern", "return", "union", "const", "float", "short", "unsigned", "continue", "for", "signed", "void", "default", "goto", "sizeof", "volatile", "do", "if", "static", "while"};

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