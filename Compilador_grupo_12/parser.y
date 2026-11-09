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
%token LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE SEMI ASSIGN COMMA
%token IF ELSE FOR WHILE RETURN
%token INT CHAR BOOL FLOAT VOID

%type <node> program stmt expr_stmt decl_stmt assign_stmt expr
%type <node> func_decl param_list param args_list block_stmt
%type <node> if_stmt while_stmt for_stmt
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
    | func_decl   { $$ = $1; }
    | block_stmt  { $$ = $1; }
    | if_stmt     { $$ = $1; }
    | while_stmt  { $$ = $1; }
    | for_stmt    { $$ = $1; }
    | RETURN expr SEMI { $$ = ast_new_return($2); }
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
    | type ID LBRACK NUM RBRACK SEMI {
        if ($4 <= 0) {
            fprintf(stderr, "[ERRO SEMANTICO] O tamanho do vetor '%s' deve ser positivo.\n", $2);
            $$ = NULL;
        } else {
            Symbol *s = insert_symbol($2, $1, 1, $4, current_scope);
            if (!s) {
                fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' ja declarado no escopo %d.\n", $2, current_scope);
                $$ = NULL;
            } else {
                $$ = ast_new_array_decl($2, $1, $4);
            }
        }
        free($2);
    }
    ;

assign_stmt:
      ID ASSIGN expr SEMI {
        Symbol *s = lookup_symbol($1, current_scope);
        if (!s) {
            fprintf(stderr, "[ERRO SEMANTICO] Atribuicao: Variavel '%s' nao foi declarada!\n", $1);
            $$ = NULL;
        } else if (!$3) {
            $$ = NULL;
        } else if (s->type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Atribuicao: Tipos incompativeis para a variavel '%s'.\n", $1);
            $$ = NULL;
        } else {
            $$ = ast_new_assign($1, $3);
        }
        free($1);
    }
    | ID LBRACK expr RBRACK ASSIGN expr SEMI {
        Symbol *s = lookup_symbol($1, current_scope);
        int valid = s != NULL && s->is_array && $3 != NULL && $6 != NULL;
        if (!s) {
            fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' nao foi declarado.\n", $1);
        } else if (!s->is_array) {
            fprintf(stderr, "[ERRO SEMANTICO] '%s' nao e um vetor.\n", $1);
        }
        if (!$3 || $3->expr_type != TYPE_INT) {
            fprintf(stderr, "[ERRO SEMANTICO] O indice de vetor '%s' deve ser um inteiro.\n", $1);
            valid = 0;
        } else if (s && s->is_array && $3->type == AST_NUM &&
                   ($3->intval < 0 || $3->intval >= s->array_size)) {
            fprintf(stderr, "[ERRO SEMANTICO] Indice %d fora dos limites do vetor '%s'.\n", $3->intval, $1);
            valid = 0;
        }
        if (s && s->is_array && $6 && s->type != $6->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Tipo incompativel ao atribuir em '%s'.\n", $1);
            valid = 0;
        }

        $$ = valid ? ast_new_array_assign($1, $3, $6) : NULL;
        free($1);
    }
    ;

type:
      INT   { $$ = TYPE_INT; }
    | CHAR  { $$ = TYPE_CHAR; }
    | FLOAT { $$ = TYPE_FLOAT; }
    ;

expr:
      expr PLUS expr      {
        if (!$1 || !$3 || $1->expr_type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Incompatibilidade de tipos na soma!\n");
            $$ = ast_new_binop(PLUS, $1, $3);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_binop(PLUS, $1, $3);
            $$->expr_type = $1->expr_type;
        }
      }
    | expr MINUS expr     {
        if (!$1 || !$3 || $1->expr_type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Incompatibilidade de tipos na subtração!\n");
            $$ = ast_new_binop(MINUS, $1, $3);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_binop(MINUS, $1, $3);
            $$->expr_type = $1->expr_type;
        }
      }
    | expr TIMES expr     {
        if (!$1 || !$3 || $1->expr_type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Incopatibilidade de tipos na multiplicação!\n");
            $$ = ast_new_binop(TIMES, $1, $3);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_binop(TIMES, $1, $3);
            $$->expr_type = $1->expr_type;
        }
      }
    | expr DIVIDE expr    {
        if (!$1 || !$3 || $1->expr_type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Incopatibilidade de tipos na divisão!\n");
            $$ = ast_new_binop(DIVIDE, $1, $3);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_binop(DIVIDE, $1, $3);
            $$->expr_type = $1->expr_type;
        }
      }
    | LPAREN expr RPAREN  { $$ = $2; }
    | NUM                 { $$ = ast_new_num($1); }
    | ID                  {
        Symbol *s = lookup_symbol($1, current_scope);
        if (!s) {
            fprintf(stderr, "[ERRO SEMANTICO] Expressao: Variavel '%s' nao declarada!\n", $1);
            $$ = ast_new_id($1, TYPE_UNKNOWN);
        } else {
            $$ = ast_new_id($1, s->type);
        }
        free($1);
      }
    | ID LBRACK expr RBRACK {
        Symbol *s = lookup_symbol($1, current_scope);
        if (!s) {
            fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' nao foi declarado.\n", $1);
            $$ = NULL;
        } else if (!s->is_array) {
            fprintf(stderr, "[ERRO SEMANTICO] '%s' nao e um vetor.\n", $1);
            $$ = NULL;
        } else if (!$3 || $3->expr_type != TYPE_INT) {
            fprintf(stderr, "[ERRO SEMANTICO] O indice de vetor '%s' deve ser inteiro.\n", $1);
            $$ = NULL;
        } else if ($3->type == AST_NUM &&
                   ($3->intval < 0 || $3->intval >= s->array_size)) {
            fprintf(stderr, "[ERRO SEMANTICO] Indice %d fora dos limites do vetor '%s'.\n", $3->intval, $1);
            $$ = NULL;
        } else {
            $$ = ast_new_array_access($1, $3, s->type);
        }
        free($1);
      }
    | ID LPAREN args_list RPAREN { $$ = ast_new_func_call($1, $3); }
    ;

func_decl:
      type ID LPAREN { current_scope++; } param_list RPAREN LBRACE program RBRACE {
          close_scope(current_scope);
          current_scope--;
          $$ = ast_new_func_decl($2, $1, $5, $8);
          free($2);
      }
    ;

param_list:
      /* vazio */            { $$ = NULL; }
    | param                  { $$ = $1; }
    | param_list COMMA param { $$ = ast_new_param_list($1, $3); }
    ;

param:
      type ID {
          Symbol *s = insert_symbol($2, $1, 0, 0, current_scope);
          if (!s)
              fprintf(stderr, "[ERRO SEMANTICO] Parametro '%s' duplicado no escopo %d.\n", $2, current_scope);
          $$ = ast_new_param($2, $1);
          free($2);
      }
    ;

args_list:
      /* vazio */            { $$ = NULL; }
    | expr                   { $$ = $1; }
    | args_list COMMA expr   { $$ = ast_new_args_list($1, $3); }
    ;

block_stmt:
      LBRACE { current_scope++; } program RBRACE {
          close_scope(current_scope);
          current_scope--;
          $$ = $3;
      }
    ;

if_stmt:
      IF LPAREN expr RPAREN stmt %prec LOWER_THAN_ELSE {
        if (!$3 || ($3->expr_type != TYPE_BOOL && $3->expr_type != TYPE_INT)) {
           fprintf(stderr, "[ERRO SEMANTICO] A condição do if deve ser booleana ou inteira\n");
        }
        $$ = ast_new_if($3, $5);
      }
    | IF LPAREN expr RPAREN stmt ELSE stmt {
        if (!$3 || ($3->expr_type != TYPE_BOOL && $3->expr_type != TYPE_INT)) {
           fprintf(stderr, "[ERRO SEMANTICO] A condição do if deve ser booleana ou inteira\n");
        }
        $$ = ast_new_if_else($3, $5, $7);
      }
    ;

while_stmt:
      WHILE LPAREN expr RPAREN stmt {
        if ($3 != NULL && $3->expr_type != TYPE_BOOL && $3->expr_type != TYPE_INT) {
           fprintf(stderr, "[ERRO SEMANTICO] A condição do while deve ser booleana ou inteira\n");
        }
        $$ = ast_new_while($3, $5);
      }
    ;

for_stmt:
      FOR LPAREN { current_scope++; } expr_stmt expr_stmt expr RPAREN stmt {
        close_scope(current_scope);
        current_scope--;
        if ($5 != NULL && $5->expr_type != TYPE_BOOL && $5->expr_type != TYPE_INT) {
           fprintf(stderr, "[ERRO SEMANTICO] A condição do for deve ser booleana ou inteira\n");
        }
        $$ = ast_new_for($4, $5, $6, $8);
      }
    | FOR LPAREN { current_scope++; } decl_stmt expr_stmt expr RPAREN stmt {
        close_scope(current_scope);
        current_scope--;
        if ($5 != NULL && $5->expr_type != TYPE_BOOL && $5->expr_type != TYPE_INT) {
           fprintf(stderr, "[ERRO SEMANTICO] A condição do for deve ser booleana ou inteira\n");
        }
        $$ = ast_new_for($4, $5, $6, $8);
      }
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
