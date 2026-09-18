%{
#include <stdio.h>
#include <stdlib.h>
#include "parser.tab.h"
%}

digit [0-9]
letter [a-zA-Z]
under [_]
number {digit}+

id ({letter}|{under})({letter}|{digit}|{under})*

%%

{number}{letter}+       { printf("'%s' eh invalido", yytext); exit(1); }
{number}          { yylval.intval = atoi(yytext); return NUM; }
{id}            { yylval.id = strdup(yytext); return ID; }
"+"             { return PLUS; }
"-"         { return MINUS; }
"*"         { return TIMES; }
"/"         { return DIVIDE; }
"("         { return LPAREN; }
")"         { return RPAREN; }
";"         { return SEMI; }

[ \t\n\r]+  { }

.           { printf("Caractere nao reconhecido: %s\n", yytext); }

%%

int yywrap(void) {
        return 1;
}
