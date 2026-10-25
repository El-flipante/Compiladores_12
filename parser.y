%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(void);
void yyerror(const char *s);
%}

%union {
        int intval;
        char *id;
}

%token <intval> NUM
%token <id> ID

/* Operadores */
%token PLUS MINUS TIMES DIVIDE MOD
%token LPAREN RPAREN LBRACK RBRACK SEMI ASSIGN

/* Controle de Fluxo */
%token IF ELSE FOR WHILE RETURN

/* Tipos */
%token INT CHAR BOOL FLOAT VOID

/* Precedencia e Associatividade */
%right ASSIGN
%left PLUS MINUS
%left TIMES DIVIDE MOD

%type <intval> expr

%start program

%%

program:
      /* Vazio */
    | program stmt
    ;

stmt:
      expr_stmt
    | decl_stmt
    | assign_stmt
    | if_stmt
    | while_stmt
    | for_stmt
    | return_stmt
    | error SEMI {
        fprintf(stderr, "[ERRO SINTATICO] Erro recuperado ate ';'.\n");
        yyerrok;
        yyclearin;
    }
    ;

expr_stmt:
      expr SEMI { printf("Expressao avaliada: %d\n", $1); }
    ;

decl_stmt:
      type ID SEMI { printf("Declaracao de variavel: %s\n", $2); free($2); }
    | type ID ASSIGN expr SEMI { printf("Declaracao e atribuicao: %s = %d\n", $2, $4); free($2); }
    | type ID LBRACK expr RBRACK SEMI { printf("Declaracao de array: %s[%d]\n", $2, $4); free($2); }
    ;

type:
      INT | CHAR | BOOL | FLOAT | VOID
    ;

assign_stmt:
      ID ASSIGN expr SEMI { printf("Atribuicao: %s = %d\n", $1, $3); free($1); }
    | ID LBRACK expr RBRACK ASSIGN expr SEMI { printf("Atribuicao em array: %s[%d] = %d\n", $1, $3, $6); free($1); }
    ;

if_stmt:
      IF LPAREN expr RPAREN stmt
    | IF LPAREN expr RPAREN stmt ELSE stmt
    ;

while_stmt:
      WHILE LPAREN expr RPAREN stmt
    ;

for_stmt:
      FOR LPAREN expr_stmt expr_stmt expr RPAREN stmt
    | FOR LPAREN decl_stmt expr_stmt expr RPAREN stmt
    ;

return_stmt:
      RETURN expr SEMI { printf("Comando return identificado.\n"); }
    | RETURN SEMI { printf("Comando return vazio identificado.\n"); }
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
    | expr MOD expr       {
          if ($3 == 0) {
             fprintf(stderr, "[ERRO SEMANTICO] Modulo por zero!\n");
             $$ = 0; 
          } else {
             $$ = $1 % $3;
          }
        }
    | LPAREN expr RPAREN  { $$ = $2; }
    | NUM                 { $$ = $1; }
    | ID                  { 
          /* Simulando uma leitura na tabela de simbolos (assumindo 0 por seguranca) */
          printf("Lendo variavel %s\n", $1); 
          $$ = 0; 
          free($1);
        }
    ;

%%

int main() {
    printf("rodando o parser atualizado... \n");
    return yyparse();
}

void yyerror(const char *s) {
    fprintf(stderr, "erro sintatico: %s\n", s);
}