%code top { 
    #include "include/symbol_types.h"
    #include "include/types.h"
}

%{

#ifdef ME
#include "include/tree_procedures.h"
#else
#include "tree_procedures.c"
#endif

#ifndef YYDEBUG

#define DO_TREE_OPS

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif


#if defined DO_TREE_OPS && defined ME
#include "include/colours.h"
#include "include/codegen.h"
#include "include/optimise_tree.h"
#include "include/splio.h"
#include "include/symbol_table.h"
#elif defined DO_TREE_OPS
#include "include/colours.h"
#include "include/splio.h"
#include "symbol_table.c"
#include "utils.c"
#include "codegen.c"
#include "optimise_tree.c"
#include "types.c"
#endif

int yycolumn = 1;

/* ------------- forward declarations --------------------------- */


void yyerror(char *);
extern int yylex(void);

extern int yylineno;
extern int *lineno;
extern int *colno;
%}

%locations

%defines

/* Include types.h again so that it is included in the bison header file */
%code requires { 
    #include "include/types.h"
    extern int yycolumn;
}

/****************/
/* Start symbol */
/****************/

%start  program

/**********************/
/* Action value types */
/**********************/

%union {
    int iVal;
    TERNARY_TREE  tVal;
}

%token COLON FULLSTOP SEMICOLON COMMA ASSIGN BRA KET PLUS MINUS MULTIPLY DIVIDE 
%token EQUAL_TO NEQUAL_TO LESS_THAN GREATER_THAN LESS_THAN_EQUAL GREATER_THAN_EQUAL 
%token APOSTROPHE DECLARATIONS CODE OF TYPE CHARACTER INTEGER REAL 
%token IF THEN ELSE ENDIF DO WHILE ENDDO ENDWHILE FOR IS BY TO ENDFOR WRITE NEWLINE READ 
%token NOT AND OR ENDP INVALID

/* These are the types of lexical tokens -> iVal */
%token<iVal> IDENTIFIER INT CHAR FLOAT

/* Whereas Rules return a tVal type (Tree) */
%type<tVal> program block declaration_block declaration identifier_list identifier
%type<tVal> type statement_list statement assignment_statement if_statement do_statement
%type<tVal> while_statement for_statement for_assign for_props loop_body
%type<tVal> write_statement read_statement output_list conditional comparison comparator
%type<tVal> expression term value constant number_constant

%%
program                 :  identifier  COLON  block  ENDP  identifier  FULLSTOP
                        {
                        #ifdef DO_TREE_OPS
                            TERNARY_TREE ParseTree;
                            ParseTree = create_inode(NOTHING, PROGRAM, $1, $3, $5);
                        #ifdef DEBUG
                            PrintTree(ParseTree, 0);
                        #endif
                            Optimise(&ParseTree);
                        #ifdef DEBUG
                            PrintTree(ParseTree, 0);
                        #else
                            int retVal = 0;
                            const char *temp_dir = "spl_compiler";
                            const char *extension = ".c";
                            time_t seconds_since_epoch = time(NULL);
                            long ticks = seconds_since_epoch*CLOCKS_PER_SEC;
                            INFO("Ticks since epoch, %ld\n", ticks)
                            size_t name_length = 
                                strlen(temp_dir) + snprintf(NULL, 0, "%ld", ticks) + strlen(extension) + 1;
                            char *file_name = (char *)malloc(name_length);
                            INFO("Name length %zd, ptr is %p\n", name_length, file_name)
                            snprintf(file_name, name_length, "%s%ld%s", temp_dir, ticks, extension);
                            INFO("Creating file \"%s\"\n", file_name)
                            FILE *output = fopen(file_name, "a");
                            INFO("File name is %s, pointer is %p\n", file_name, output)
                            INFO("Generating code..\n")
                            lineno = &yylineno;
                            colno  = &yycolumn;
                            retVal = GenerateC(ParseTree, 0, output);
                            fclose(output);
                            if(retVal >  -1) {
                                output = fopen(file_name, "r");
                                char* read_buf = (char *)malloc(100);
                                INFO("Attempting to print code..\n")
                                while(fgets(read_buf, 100, output)!=NULL)
                                {
                                    printf("%s", read_buf);
                                }
                                fclose(output);
                                free(read_buf);
                            }
                            else printf("Compilation failed.\n");
                            int ret = remove(file_name);
                            if(!ret)
                            {
                                INFO("Temp file removed successfully.\n")
                            }
                            free(file_name);
                        #endif /*    DEBUG    */
                        #endif /* DO_TREE_OPS */
                        }
                        ;
 
block                   : DECLARATIONS  declaration_block  CODE  statement_list
                        {
                            $$ = create_inode(NOTHING, BLOCK, $2, $4, NULL);
                        }  
                        | CODE  statement_list
                        {
                            $$ = create_inode(NOTHING, BLOCK, $2, NULL, NULL);
                        }
                        ;
 
declaration_block       :  declaration
                        {
                            $$ = create_inode(NOTHING, DECLARATION_BLOCK, $1, NULL, NULL);
                        }
                        |  declaration  declaration_block
                        {
                            $$ = create_inode(NOTHING, DECLARATION_BLOCK, $1, $2, NULL);
                        }
                        ;
 
declaration             :  identifier_list  OF TYPE  type  SEMICOLON
                        {
                            $$ = create_inode(NOTHING, DECLARATION, $1, $4, NULL);
                        }
                        ;
 
identifier_list         :  identifier
                        {
                            $$ = create_inode(NOTHING, ID_LIST, $1, NULL, NULL);
                        }
                        |  identifier  COMMA  identifier_list
                        {
                            $$ = create_inode(NOTHING, ID_LIST, $1, $3, NULL);
                        }
                        ;
 
type                    : CHARACTER
                        {
                            $$ = create_inode(CHAR_T, TYPE_P, NULL, NULL, NULL);
                        }
                        | INTEGER
                        {
                            $$ = create_inode(INT_T, TYPE_P, NULL, NULL, NULL);
                        }
                        | REAL
                        {
                            $$ = create_inode(REAL_T, TYPE_P, NULL, NULL, NULL);
                        }
                        ;
 
statement_list          :  statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT_LIST, $1, NULL, NULL);
                        }
                        |  statement  SEMICOLON  statement_list
                        {
                            $$ = create_inode(NOTHING, STATEMENT_LIST, $1, $3, NULL);
                        }
                        ;
 
statement               :  assignment_statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT, $1, NULL, NULL);
                        }
                        |  if_statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT, $1, NULL, NULL);
                        }
                        |  do_statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT, $1, NULL, NULL);
                        }
                        |  while_statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT, $1, NULL, NULL);
                        }
                        |  for_statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT, $1, NULL, NULL);
                        }
                        |  write_statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT, $1, NULL, NULL);
                        }
                        |  read_statement
                        {
                            $$ = create_inode(NOTHING, STATEMENT, $1, NULL, NULL);
                        }
                        ;
 
assignment_statement    :  expression  ASSIGN  identifier
                        {
                            $$ = create_inode(NOTHING, ASSIGNMENT, $1, $3, NULL);
                        }
                        ;
 
if_statement            : IF  conditional  THEN  statement_list  ENDIF
                        {
                            $$ = create_inode(NOTHING, IF_S, $2, $4, NULL);
                        }
                        | IF  conditional  THEN  statement_list  ELSE  statement_list  ENDIF
                        {
                            $$ = create_inode(NOTHING, IF_S, $2, $4, $6);
                        }
                        ;
 
do_statement            : loop_body  WHILE  conditional  ENDDO
                        {
                            $$ = create_inode(NOTHING, DO_S, $1, $3, NULL);
                        }
                        ;
 
while_statement         : WHILE  conditional loop_body  ENDWHILE
                        {
                            $$ = create_inode(NOTHING, WHILE_S, $2, $3, NULL);
                        }
                        ;
 
for_statement           : FOR  for_assign  for_props  loop_body  ENDFOR
                        {
                            $$ = create_inode(NOTHING, FOR_S, $2, $3, $4);
                        }
                        ;

for_assign              : identifier IS expression
                        {
                            $$ = create_inode(NOTHING, FOR_ASSIGN, $1, $3, NULL);
                        }

for_props               : BY expression TO expression
                        {
                            $$ = create_inode(NOTHING, FOR_PROPERTIES, $2, $4, NULL);
                        }


loop_body               : DO statement_list
                        {
                            $$ = create_inode(NOTHING, LOOP_BODY, $2, NULL, NULL);
                        }
 
write_statement         : WRITE BRA  output_list  KET
                        {
                            $$ = create_inode(NOTHING, WRITE_S, $3, NULL, NULL);
                        }
                        | NEWLINE
                        {
                            $$ = create_inode(NOTHING, WRITE_NEWLINE, NULL, NULL, NULL);
                        }
                        ;
 
read_statement          : READ BRA  identifier  KET
                        {
                            $$ = create_inode(NOTHING, READ_S, $3, NULL, NULL);
                        }
                        ;
 
output_list             :  value
                        {
                            $$ = create_inode(NOTHING, OUTPUT_LIST, $1, NULL, NULL);
                        } 
                        |  value  COMMA  output_list
                        {
                            $$ = create_inode(NOTHING, OUTPUT_LIST, $1, $3, NULL);
                        }
                        ;
 
conditional             :  comparison
                        {
                            $$ = create_inode(NOTHING, CONDITIONAL, $1, NULL, NULL);
                        }  
                        |  NOT conditional
                        {
                            $$ = create_inode(NOTHING, NEGATION, $2, NULL, NULL);
                        }
                        |  comparison   AND  conditional
                        {
                            $$ = create_inode(NOTHING, LOG_AND, $1, $3, NULL);
                        }
                        |  comparison  OR  conditional
                        {
                            $$ = create_inode(OR, LOG_OR, $1, $3, NULL);
                        }
                        ;

comparison              :  expression comparator expression
                        {
                            $$ = create_inode(NOTHING, COMPARISON, $1, $2, $3);
                        }
                        ;
 
comparator              : EQUAL_TO
                        {
                            $$ = create_inode(SYM_EQ_TO, COMPARATOR, NULL, NULL, NULL);
                        }
                        | NEQUAL_TO
                        {
                            $$ = create_inode(SYM_NEQ_TO, COMPARATOR, NULL, NULL, NULL);
                        }
                        | LESS_THAN
                        {
                            $$ = create_inode(SYM_LESS_THAN, COMPARATOR, NULL, NULL, NULL);
                        }
                        | GREATER_THAN
                        {
                            $$ = create_inode(SYM_GREATER_THAN, COMPARATOR, NULL, NULL, NULL);
                        }
                        | LESS_THAN_EQUAL
                        {
                            $$ = create_inode(SYM_LESS_THAN_EQ, COMPARATOR, NULL, NULL, NULL);
                        }
                        | GREATER_THAN_EQUAL
                        {
                            $$ = create_inode(SYM_GREATER_THAN_EQ, COMPARATOR, NULL, NULL, NULL);
                        }
                        ;
 
expression              : term
                        {
                            $$ = create_inode(NOTHING, EXPRESSION, $1, NULL, NULL);
                        }
                        | term  PLUS  expression
                        {
                            $$ = create_inode(NOTHING, EXPR_ADD, $1, $3, NULL);
                        }
                        |  term  MINUS  expression
                        {
                            $$ = create_inode(MINUS, EXPR_MINUS, $1, $3, NULL);
                        };
 
term                    :  value
                        {
                            $$ = create_inode(NOTHING, TERM, $1, NULL, NULL);
                        }
                        |  value  MULTIPLY  term
                        {
                            $$ = create_inode(MULTIPLY, TERM_MUL, $1, $3, NULL);
                        }
                        |  value  DIVIDE  term
                        {
                            $$ = create_inode(DIVIDE, TERM_DIV, $1, $3, NULL);
                        }
                        ;
 
value                   : identifier
                        {
                            $$ = create_inode(NOTHING, VAL_IDENTIFIER, $1, NULL, NULL);
                        }
                        | constant
                        {
                            $$ = create_inode(NOTHING, VAL_CONSTANT, $1, NULL, NULL);
                        }
                        | BRA  expression  KET
                        {
                            $$ = create_inode(NOTHING, VAL_EXPR, $2, NULL, NULL);
                        };
 
constant                :  number_constant
                        {
                            $$ = create_inode(NOTHING, NUMBER_CONST, $1, NULL, NULL);
                        }
                        |  CHAR
                        {
                            $$ = create_inode($1, CHAR_CONST, NULL, NULL, NULL);
                        }
                        ;
 
number_constant         : INT
                        {
                            $$ = create_inode($1, INT_CONST, NULL, NULL, NULL);
                        }
                        | MINUS INT
                        {
                            $$ = create_inode($2, NEG_INT_CONST, NULL, NULL, NULL);
                        }
                        | FLOAT
                        {
                            $$ = create_inode($1, FLOAT_CONST, NULL, NULL, NULL);
                        }
                        | MINUS FLOAT
                        {
                            $$ = create_inode($2, NEG_FLOAT_CONST, NULL, NULL, NULL);
                        }
                        ;

identifier              : IDENTIFIER
                        {
                            $$ = create_inode($1, ID_VAL, NULL, NULL, NULL);
                        }

%%

#ifndef ME
#include "lex.yy.c"
#endif