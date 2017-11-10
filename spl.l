%{
#ifdef DO_TREE_OPS
#define INSTALL_SYM(id, type) yylval.iVal = installId(id, type);
#else
#define INSTALL_SYM(id, type)
#endif
#ifdef PRINT
#define         TOKEN(t) printf("Token: " #t "\n");
#define         ID_TOKEN(t) printf("Token: " #t " Identifier Value: %s\n", yytext);
#define         INT_TOKEN(t) printf("Token: " #t " Integer Value: %d\n", atoi(yytext));
#define         FLOAT_TOKEN(t) printf("Token: " #t " Float Value: %f\n", atof(yytext));
#define         CHAR_TOKEN(t) printf("Token: " #t " Character Value: %c\n", yytext[1]);
#define         INVALID_TOKEN printf("The specified token is not valid within the SPL language:%s\n", yytext);
#define         NEWLINE_TOKEN
#else
#define         TOKEN(t) return (t);
#define         ID_TOKEN(t) INSTALL_SYM(yytext, UNKNOWN_T) return(t); 
#define         INT_TOKEN(t) yylval.iVal = atoi(yytext); return(t);
#define         FLOAT_TOKEN(t) INSTALL_SYM(yytext, REAL_T) return(t);
#define         CHAR_TOKEN(t) yylval.iVal = yytext[1]; return(t);
#define         INVALID_TOKEN return (INVALID);
#define         NEWLINE_TOKEN yycolumn = 1;

#include <ctype.h>
#include <string.h>
#include "include/splio.h"
#include "include/symbol_table.h"

#ifdef ME
#include "spl.tab.h"
#else
extern DYNAMIC_SYMTAB *symTabRec;
#endif

/* 
** Implement a line/column tracker.
** Taking inspiration from https://stackoverflow.com/a/8024849 
*/

/* yycolumn is declared in spl.tab.h
** int yycolumn = 1; */
extern int yycolumn;

#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno; \
                       yylloc.first_column = yycolumn; yylloc.last_column = yycolumn + yyleng -1; \
                       yycolumn += yyleng;

int installId(char *, enum SymbolTypes);


#endif
%}

%option yylineno

ws              [ \t\r]
newline         \n
delim           {ws}+
digit           [0-9]
character       [a-zA-Z]
char_const      \'{character}\'
number          {digit}+
floating_point  {number}\.{number}
id              {character}({character}|{digit})*
everything      .
%%
{delim}          ;
\n               NEWLINE_TOKEN;
":"              TOKEN(COLON)
"."              TOKEN(FULLSTOP)
";"              TOKEN(SEMICOLON)
","              TOKEN(COMMA)
"->"             TOKEN(ASSIGN)

"("              TOKEN(BRA)
")"              TOKEN(KET)
"+"              TOKEN(PLUS)
"-"              TOKEN(MINUS)
"*"              TOKEN(MULTIPLY)
"/"              TOKEN(DIVIDE)

"="              TOKEN(EQUAL_TO)
"<>"             TOKEN(NEQUAL_TO)
"<"              TOKEN(LESS_THAN)
">"              TOKEN(GREATER_THAN)
"<="             TOKEN(LESS_THAN_EQUAL)
">="             TOKEN(GREATER_THAN_EQUAL)

DECLARATIONS     TOKEN(DECLARATIONS)
CODE             TOKEN(CODE)

OF               TOKEN(OF)
TYPE             TOKEN(TYPE)
CHARACTER        TOKEN(CHARACTER)
INTEGER          TOKEN(INTEGER)
REAL             TOKEN(REAL)

IF               TOKEN(IF)
THEN             TOKEN(THEN)
ELSE             TOKEN(ELSE)
ENDIF            TOKEN(ENDIF)

DO               TOKEN(DO)
WHILE            TOKEN(WHILE)
ENDDO            TOKEN(ENDDO)
ENDWHILE         TOKEN(ENDWHILE)

FOR              TOKEN(FOR)
IS               TOKEN(IS)
BY               TOKEN(BY)
TO               TOKEN(TO)
ENDFOR           TOKEN(ENDFOR)

WRITE            TOKEN(WRITE)
NEWLINE          TOKEN(NEWLINE)
READ             TOKEN(READ)

NOT              TOKEN(NOT)
AND              TOKEN(AND)
OR               TOKEN(OR)

ENDP             TOKEN(ENDP)

{number}         INT_TOKEN(INT)
{floating_point} FLOAT_TOKEN(FLOAT)
{char_const}     CHAR_TOKEN(CHAR)
{id}             ID_TOKEN(IDENTIFIER)
{everything}     INVALID_TOKEN;
%%

/* Here is the code for the library of symbol table routines */

/* code for a simple symbol table, which is an array of pointers to
   structs, each of which contains an identifier.
*/

#if !defined PRINT && defined DO_TREE_OPS
SYMTABNODEPTR newSymTabNode()
{
    return ((SYMTABNODEPTR)malloc(sizeof(SYMTABNODE)));
}

/* Look up an identifier in the symbol table, if its there return
   its index.  If its not there, put it in the end position,
   as long as the table isn't full, and return its index.
*/

int installId(char *id, enum SymbolTypes type) 
{
    SYMTABNODEPTR *symTab;
    int index;
    SYMTABNODEPTR new;
    if(symTabRec == NULL)
        symTabRec = create_dynamic_symtab();
    symTab = symTabRec->array;
    INFO("Found identifier: %s, length: %zds\n", id, strlen(id));
    index = lookup_symbol(id, symTabRec);
    if (index < 0)
    {
        new = newSymTabNode();
        new->identifier = (char *)malloc(strlen(id));
        if(isdigit(*id)) {
            new->declared = TRUE;
            new->initialised = TRUE;
            new->sanitised = TRUE;
        }
        INFO("Identifier pointer created for symbol %d ", index);
        
        strcpy(new->identifier,id);
        INFO("at %p, id: %s\n\n", new->identifier, new->identifier);
        new->type = type;
        index = add_symbol(symTabRec, new);
        /*symTab[index]->identifier[strlen(id)] = '\0';*/
        return(index);
    }
    return index;
}

#endif