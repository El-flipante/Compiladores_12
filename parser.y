%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "ast.h" // Incluir a AST

int yylex(void);
void yyerror(const char *s);

int current_scope = 0;
ASTNode *ast_root = NULL; // Raiz da árvore
%}

%union {
    int intval;
    char *id;
    DataType data_type;
    struct ASTNode *node; // Adicionado ponteiro para nó da AST
}

%token <intval> NUM
%token <id> ID
%token PLUS MINUS TIMES DIVIDE MOD
%token LPAREN RPAREN LBRACK RBRACK SEMI ASSIGN
%token IF ELSE FOR WHILE RETURN
%token INT CHAR BOOL FLOAT VOID

%type <node> program stmt expr_stmt decl_stmt assign_stmt expr
%type <data_type> type 

%right ASSIGN
%left PLUS MINUS
%left TIMES DIVIDE MOD

%start program

%%

program:
       { $$ = NULL; ast_root = NULL; }
    | program stmt {
        $$ = ast_new_seq($1, $2);
        ast_root = $$; // Salva o topo da árvore
    }
    ;

stmt:
      expr_stmt   { $$ = $1; }
    | decl_stmt   { $$ = $1; }
    | assign_stmt { $$ = $1; }
    ;

expr_stmt:
      expr SEMI { $$ = $1; }
    ;

decl_stmt:
      type ID SEMI {
        Symbol *s = insert_symbol($2, $1, 0, 0, current_scope);
        if (!s) {
            fprintf(stderr, "[ERRO SEMÂNTICO] Variável '%s' já declarada no escopo %d.\n", $2, current_scope);
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
            fprintf(stderr, "[ERRO SEMÂNTICO] Atribuição: Variável '%s' não foi declarada!\n", $1);
            $$ = NULL;
        } else if (s->type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMÂNTICO] Atribuição: Tipos incompativeis para a variável '%s'.\n", $1);
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
            fprintf(stderr, "[ERRO SEMÂNTICO] Expressão: Variável '%s' não declarada!\n", $1);
            $$ = ast_new_id($1, TYPE_UNKNOWN); // Evita falha de segmentação no resto da árvore
        } else {
            $$ = ast_new_id($1, s->type);
        }
        free($1);
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