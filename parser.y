%{
#include <stdio.h>
#include <stdlib.h>


int yylex(void);
void yyerror(const char *s);
%}

%union {
        int intval;
        char *id;
}

%token <intval> NUM
%token <id> ID
%token PLUS MINUS TIMES DIVIDE LPAREN RPAREN SEMI
%type <intval> expr

%left PLUS MINUS
%left TIMES DIVIDE

%start input

%%

input:
     | input expr SEMI          { printf("identificado SEMI: expr = %d\n", $2); }
     | input error SEMI         {
        fprintf(stderr, "[ERRO SINTATICO] Erro recuperado ate ';'\n");
        yyerrok;
        yyclearin;
        }
        ;

expr:
      expr PLUS expr      { $$ = $1 + $3; }
    | expr MINUS expr     { $$ = $1 - $3; }
    | expr TIMES expr     { $$ = $1 * $3; }
    | expr DIVIDE expr    { 
          if ($3 == 0) {
             fprintf(stderr, "[ERRO SEMANTICO] Divisao por zero!\n");
             $$ = 0; 
          } else {
             $$ = $1 / $3;
          }
        }
    | LPAREN expr RPAREN   { $$ = $2; }
    | NUM                  { $$ = $1; }
    ;

%%

int main() {
        printf("rodando... \n");
        return yyparse();
}

void yyerror(const char *s) {
        fprintf(stderr, "erro sintatico: %s\n", s);
}
