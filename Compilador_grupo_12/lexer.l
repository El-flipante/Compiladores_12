%{
#include <stdio.h>
#include <stdlib.h>
#include "symtab.h"
#include "parser.tab.h"
%}

%option nounput noinput

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

\"([^\"\\]|\\.)*\"  { 
    yylval.id = strdup(yytext); 
    return STRING_LITERAL; 
}

{number}{letter}+       { fprintf(stderr, "[ERRO LEXICO] Identificador invalido: '%s'\n", yytext); exit(1); }
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
"{"        { return LBRACE; }
"}"        { return RBRACE; }	    
"="	    { return ASSIGN; }
","        { return COMMA; }

"//".*
"/*"([^*]|\*+[^*/])*\*+"/"

[ \t\n\r]+  { }

.           { fprintf(stderr, "[ERRO LEXICO] Caractere invalido: %s\n", yytext); exit(1); }

%%

int yywrap(void) {
        return 1;
}