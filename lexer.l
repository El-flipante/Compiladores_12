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

"if" { return IF; }
"else" { return ELSE; }
"for" { return FOR; }
"while" { return WHILE; }
"return" { return RETURN; }
"int" { return INT; }
"char" { return CHAR; }
"bool" { return BOOL; }
"float" { return FLOAT; }
"void" { return VOID; }

{number}{letter}+       { printf("'%s' eh invalido\n", yytext); exit(1); }
{number}          { yylval.intval = atoi(yytext); return NUM; }
{id}            { yylval.id = strdup(yytext); return ID; }
"+"             { return PLUS; }
"-"         { return MINUS; }
"*"         { return TIMES; }
"/"         { return DIVIDE; }
"%"	    { return MOD; }
"("         { return LPAREN; }
")"         { return RPAREN; }
";"         { return SEMI; }
"["	    { return LBRACK; }
"]"	    { return RBRACK; }
"="	    { return ASSIGN; }

[ \t\n\r]+  { }

.           { printf("Caractere nao reconhecido: %s\n", yytext); }

%%

int yywrap(void) {
        return 1;
}
