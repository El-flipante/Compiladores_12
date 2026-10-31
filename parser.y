%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "ast.h" 

int yylex(void);
void yyerror(const char *s);

int current_scope = 0;
ASTNode *ast_root = NULL;
%}

%union {
    int intval;
    char *id;
    DataType data_type;
    struct ASTNode *node; 
}

%token <intval> NUM
%token <id> ID
%token <id> STRING_LITERAL
%token PLUS MINUS TIMES DIVIDE MOD
%token LPAREN RPAREN LBRACK RBRACK SEMI ASSIGN COMMA
%token IF ELSE FOR WHILE RETURN
%token INT CHAR BOOL FLOAT VOID

%type <node> program stmt expr_stmt decl_stmt assign_stmt expr 
%type <node> func_decl param_list param args_list 
%type <data_type> type

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGN
%left PLUS MINUS
%left TIMES DIVIDE MOD
%start program

%%

program:
       { $$ = NULL; ast_root = NULL; }
    | program stmt {
        $$ = ast_new_seq($1, $2);
        ast_root = $$; 
    }
    ;

stmt:
      expr_stmt   { $$ = $1; }
    | decl_stmt   { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | IF LPAREN expr RPAREN stmt %prec LOWER_THAN_ELSE { 
        $$ = ast_new_if($3, $5); 
    }
    | IF LPAREN expr RPAREN stmt ELSE stmt { 
        $$ = ast_new_if_else($3, $5, $7); 
    }
    | error SEMI { 
        yyerrok; 
        fprintf(stderr, "[AVISO] Recuperado de erro sintatico antes do ponto e virgula.\n"); 
        $$ = NULL; 
    }
    ;

expr_stmt:
      expr SEMI { $$ = $1; }
    ;

decl_stmt:
      type ID SEMI {
        Symbol *s = insert_symbol($2, $1, 0, 0, current_scope);
        if (!s) { 
            fprintf(stderr, "[ERRO SEMANTICO] Variavel '%s' ja declarada no escopo %d.\n", $2, current_scope);
            $$ = NULL;
        } else {
            $$ = ast_new_decl($2, $1);
        }
        free($2);
    }
    ;

assign_stmt:
      ID ASSIGN expr SEMI {
        Symbol *s = lookup_symbol_global($1);
        if (!s) {
            fprintf(stderr, "[ERRO SEMANTICO] Atribuicao: Variavel '%s' nao foi declarada!\n", $1);
            $$ = NULL;
        } else if (s->type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Atribuicao: Tipos incompativeis para a variavel '%s'.\n", $1);
            $$ = NULL;
        } else {
            $$ = ast_new_assign($1, $3);
        }
        free($1);
    }
    ;

type:
      INT   { $$ = TYPE_INT; }
    | CHAR  { $$ = TYPE_CHAR; }
    | FLOAT { $$ = TYPE_FLOAT; }
    ;

// REGRAS DE EXPRESSAO UNIFICADAS
expr:
      expr PLUS expr      { $$ = ast_new_binop(PLUS, $1, $3); }
    | expr MINUS expr     { $$ = ast_new_binop(MINUS, $1, $3); }
    | expr TIMES expr     { $$ = ast_new_binop(TIMES, $1, $3); }
    | expr DIVIDE expr    { $$ = ast_new_binop(DIVIDE, $1, $3); }
    | LPAREN expr RPAREN  { $$ = $2; }
    | NUM                 { $$ = ast_new_num($1); }
    | ID                  { 
        Symbol *s = lookup_symbol_global($1);
        if (!s) {
            fprintf(stderr, "[ERRO SEMANTICO] Expressao: Variavel '%s' nao declarada!\n", $1);
            $$ = ast_new_id($1, TYPE_UNKNOWN); 
        } else {
            $$ = ast_new_id($1, s->type);
        }
        free($1);
    }
    | ID LPAREN args_list RPAREN { $$ = ast_new_func_call($1, $3); } // CHAMADA DE FUNCAO MOVIDA PARA CA
    ;

func_decl:
      type ID LPAREN param_list RPAREN block_stmt { 
          $$ = ast_new_func_decl($2, $1, $4, $6); 
      }
    ;

param_list:
      /* vazio */            { $$ = NULL; }
    | param                  { $$ = $1; }
    | param_list COMMA param { $$ = ast_new_param_list($1, $3); }
    ;

param:
      type ID                { $$ = ast_new_param($2, $1); }
    ;

args_list:
      /* vazio */            { $$ = NULL; }
    | expr                   { $$ = $1; }
    | args_list COMMA expr   { $$ = ast_new_args_list($1, $3); }
    ;

block_stmt:
      LBRACK program RBRACK  { $$ = $2; }
    ;

%%

int main() {
    symtab_init();
    printf("Iniciando parser com Tabela de Simbolos e AST...\n");
    yyparse();
    
    printf("\n=== ARVORE SINTATICA ABSTRATA (AST) ===\n");
    ast_print(ast_root, 0);
    printf("=======================================\n");
    
    print_symtab();
    return 0;
}

void yyerror(const char *s) {
    fprintf(stderr, "erro sintatico: %s\n", s);
}
