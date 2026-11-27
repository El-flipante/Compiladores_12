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
char_literal \'([^\\'\n]|\\.)\'

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

{char_literal} {
    if (yytext[1] != '\\') {
        yylval.intval = (unsigned char) yytext[1];
    } else {
        switch (yytext[2]) {
            case '\\': yylval.intval = '\\'; break;
            case '\'': yylval.intval = '\''; break;
            case '"':  yylval.intval = '"'; break;
            case 'a':  yylval.intval = '\a'; break;
            case 'b':  yylval.intval = '\b'; break;
            case 'f':  yylval.intval = '\f'; break;
            case 'n':  yylval.intval = '\n'; break;
            case 'r':  yylval.intval = '\r'; break;
            case 't':  yylval.intval = '\t'; break;
            case 'v':  yylval.intval = '\v'; break;
            case '0':  yylval.intval = '\0'; break;
            default:
                fprintf(stderr, "[ERRO LEXICO] Escape invalido em literal de caractere: %s\n", yytext);
                exit(1);
        }
    }
    return CHAR_LITERAL;
}

\"([^\"\\]|\\.)*\"  { 
    yylval.id = strdup(yytext); 
    return STRING_LITERAL; 
}

{number}({letter}|{under})({letter}|{digit}|{under})* { fprintf(stderr, "[ERRO LEXICO] Identificador nao pode comecar com numero: '%s'\n", yytext); exit(1); }
{number}          { yylval.intval = atoi(yytext); return NUM; }
{id}        { yylval.id = strdup(yytext); return ID; }
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return TIMES; }
"/"         { return DIVIDE; }
"%"	        { return MOD; }
"<="        { return LE; }
">="        { return GE; }
"=="        { return EQ; }
"!="        { return NE; }
"<"         { return LT; }
">"         { return GT; }
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
