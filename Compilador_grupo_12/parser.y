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
DataType current_function_return_type = TYPE_UNKNOWN;
int current_function_signature_valid = 1;

int always_returns(ASTNode *node) {
    if (!node) return 0;
    switch (node->type) {
        case AST_RETURN:
            return 1;
        case AST_SEQ:
            return always_returns(node->seq.stmt1) || always_returns(node->seq.stmt2);
        case AST_IF:
            return node->if_stmt.else_branch != NULL &&
                   always_returns(node->if_stmt.if_branch) &&
                   always_returns(node->if_stmt.else_branch);
        default:
            return 0;
    }
}

ASTNode *make_comparison(int op, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_new_binop(op, left, right);
    if (!left || !right) {
        fprintf(stderr, "[ERRO SEMANTICO] Operandos invalidos na comparacao.\n");
        node->expr_type = TYPE_UNKNOWN;
    } else if (left->expr_type != right->expr_type ||
               left->expr_type == TYPE_VOID ||
               left->expr_type == TYPE_UNKNOWN) {
        fprintf(stderr, "[ERRO SEMANTICO] Tipos incompativeis na comparacao.\n");
        node->expr_type = TYPE_UNKNOWN;
    } else {
        node->expr_type = TYPE_BOOL;
    }
    return node;
}

ASTNode *make_for_assignment(const char *id, ASTNode *expr) {
    Symbol *symbol = lookup_symbol(id, current_scope);
    if (!symbol) {
        fprintf(stderr, "[ERRO SEMANTICO] Variavel '%s' nao declarada no for.\n", id);
        return NULL;
    }
    if (symbol->is_array) {
        fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' precisa de um indice no for.\n", id);
        return NULL;
    }
    if (!expr || symbol->type != expr->expr_type) {
        fprintf(stderr, "[ERRO SEMANTICO] Tipo incompativel na atribuicao de '%s' no for.\n", id);
        return NULL;
    }
    return ast_new_assign(id, expr);
}

// ----- VALIDACAO DE FUNCOES -----
DataType p_types[MAX_PARAMS];
int p_count = 0;
int p_overflow = 0;
void extract_params(ASTNode *node) {
    if (!node) return;
    if (node->type == AST_PARAM_LIST) {
        extract_params(node->param_list.next);
        extract_params(node->param_list.param);
    } else if (node->type == AST_PARAM) {
        if (p_count < MAX_PARAMS) p_types[p_count++] = node->expr_type;
        else p_overflow = 1;
    }
}
void register_function(const char *name, DataType ret_type, ASTNode *param_list_node) {
    p_count = 0;
    p_overflow = 0;
    extract_params(param_list_node);
    if (p_overflow) {
        current_function_signature_valid = 0;
        fprintf(stderr, "[ERRO SEMANTICO] Funcao '%s' excede o limite de %d parametros.\n", name, MAX_PARAMS);
        return;
    }
    Symbol *s = insert_function(name, ret_type, p_count, p_types);
    if (!s) {
        fprintf(stderr, "[ERRO SEMANTICO] Falha ao registrar funcao '%s' (ja existe ou muitos parametros).\n", name);
    }
}

DataType a_types[MAX_PARAMS];
int a_count = 0;
int a_overflow = 0;
int a_invalid = 0;
void extract_args(ASTNode *node) {
    if (!node) return;
    if (node->type == AST_ARGS_LIST) {
        extract_args(node->args_list.next);
        if (a_count < MAX_PARAMS) {
            ASTNode *arg = node->args_list.expr;
            a_types[a_count++] = arg ? arg->expr_type : TYPE_UNKNOWN;
            if (!arg || arg->expr_type == TYPE_UNKNOWN) a_invalid = 1;
        } else {
            a_overflow = 1;
        }
    } else {
        if (a_count < MAX_PARAMS) {
            a_types[a_count++] = node->expr_type;
            if (node->expr_type == TYPE_UNKNOWN) a_invalid = 1;
        } else {
            a_overflow = 1;
        }
    }
}
int validate_function_call(const char *name, ASTNode *args_list_node) {
    Symbol *shadow = lookup_symbol(name, current_scope);
    if (shadow) {
        fprintf(stderr, "[ERRO SEMANTICO] '%s' e uma variavel, nao uma funcao.\n", name);
        return 0;
    }
    Symbol *fn = lookup_function(name);
    if (!fn) {
        fprintf(stderr, "[ERRO SEMANTICO] Funcao '%s' nao declarada!\n", name);
        return 0;
    }
    a_count = 0;
    a_overflow = 0;
    a_invalid = 0;
    extract_args(args_list_node);
    if (a_overflow) {
        fprintf(stderr, "[ERRO SEMANTICO] Chamada a '%s' excede o limite de %d argumentos.\n", name, MAX_PARAMS);
        return 0;
    }
    if (a_count != fn->param_count) {
        fprintf(stderr, "[ERRO SEMANTICO] Funcao '%s' espera %d argumentos, mas %d foram passados.\n", name, fn->param_count, a_count);
        return 0;
    }
    if (a_invalid) {
        fprintf(stderr, "[ERRO SEMANTICO] Chamada a '%s' contem argumento invalido.\n", name);
        return 0;
    }
    for (int i = 0; i < a_count; i++) {
        if (a_types[i] != fn->param_types[i] && a_types[i] != TYPE_UNKNOWN && fn->param_types[i] != TYPE_UNKNOWN) {
            fprintf(stderr, "[ERRO SEMANTICO] Tipo incompativel no argumento %d da chamada a '%s'.\n", i+1, name);
            return 0;
        }
    }
    return 1;
}
%}

%union {
    int intval;
    char *id;
    DataType data_type;
    struct ASTNode *node;
}

%token <intval> NUM
%token <intval> CHAR_LITERAL
%token <id> ID
%token <id> STRING_LITERAL
%token PLUS MINUS TIMES DIVIDE MOD
%token LT GT LE GE EQ NE
%token LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE SEMI ASSIGN COMMA
%token IF ELSE FOR WHILE RETURN
%token INT CHAR BOOL FLOAT VOID

%type <node> program stmt expr_stmt decl_stmt assign_stmt expr
%type <node> func_decl param_list param args_list block_stmt
%type <node> if_stmt while_stmt for_stmt
%type <node> for_init for_condition for_update
%type <data_type> type

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGN
%nonassoc LT GT LE GE EQ NE
%left PLUS MINUS
%left TIMES DIVIDE MOD
%right UMINUS
%start program

%%

program:
       %empty { $$ = NULL; ast_root = NULL; }
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
    | RETURN expr SEMI {
        if (current_function_return_type == TYPE_UNKNOWN) {
            fprintf(stderr, "[ERRO SEMANTICO] 'return' usado fora de uma funcao.\n");
        } else if (current_function_return_type == TYPE_VOID) {
            fprintf(stderr, "[ERRO SEMANTICO] Funcao void nao pode retornar um valor.\n");
        } else if ((current_function_return_type == TYPE_INT ||
                    current_function_return_type == TYPE_CHAR) &&
                   $2 && $2->expr_type != TYPE_UNKNOWN &&
                   $2->expr_type != current_function_return_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Tipo do valor retornado difere do tipo da funcao.\n");
        }
        $$ = ast_new_return($2);
    }
    | RETURN SEMI {
        if (current_function_return_type == TYPE_UNKNOWN) {
            fprintf(stderr, "[ERRO SEMANTICO] 'return' usado fora de uma funcao.\n");
        } else if (current_function_return_type == TYPE_INT ||
                   current_function_return_type == TYPE_CHAR) {
            fprintf(stderr, "[ERRO SEMANTICO] Funcao nao void deve retornar um valor.\n");
        }
        $$ = ast_new_return(NULL);
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
        Symbol *s = NULL;
        if ($1 == TYPE_VOID) {
            fprintf(stderr, "[ERRO SEMANTICO] Variavel '%s' nao pode ter tipo void.\n", $2);
        } else {
            s = insert_symbol($2, $1, 0, 0, current_scope);
        }
        if ($1 == TYPE_VOID) {
            $$ = NULL;
        } else if (!s) {
            fprintf(stderr, "[ERRO SEMANTICO] Variavel '%s' ja declarada no escopo %d.\n", $2, current_scope);
            $$ = NULL;
        } else {
            $$ = ast_new_decl($2, $1);
        }
        free($2);
    }
    | type ID LBRACK NUM RBRACK SEMI {
        if ($1 == TYPE_VOID) {
            fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' nao pode ter tipo void.\n", $2);
            $$ = NULL;
        } else if ($4 <= 0) {
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
    | type ID LBRACK NUM RBRACK ASSIGN LBRACE args_list RBRACE SEMI { /* <--- NOVA REGRA AQUI */
        if ($1 == TYPE_VOID) {
            fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' nao pode ter tipo void.\n", $2);
            $$ = NULL;
        } else if ($4 <= 0) {
            fprintf(stderr, "[ERRO SEMANTICO] O tamanho do vetor '%s' deve ser positivo.\n", $2);
            $$ = NULL;
        } else {
            Symbol *s = insert_symbol($2, $1, 1, $4, current_scope);
            if (!s) {
                fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' ja declarado no escopo %d.\n", $2, current_scope);
                $$ = NULL;
            } else {
                // Reaproveitamos 'args_list' para a lista de inicialização separada por vírgulas
                $$ = ast_new_array_decl_init($2, $1, $4, $8); 
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
        } else if (s->is_array) {
            fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' deve ser acessado com indice.\n", $1);
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
    | BOOL  { $$ = TYPE_BOOL; }
    | VOID  { $$ = TYPE_VOID; }
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
    | MINUS expr %prec UMINUS {
        if (!$2 || ($2->expr_type != TYPE_INT && $2->expr_type != TYPE_FLOAT)) {
            fprintf(stderr, "[ERRO SEMANTICO] Operador unario '-' exige operando numerico.\n");
            $$ = ast_new_unop(MINUS, $2);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_unop(MINUS, $2);
            $$->expr_type = $2->expr_type;
        }
      }
    | expr MINUS expr     {
        if (!$1 || !$3 || $1->expr_type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Incompatibilidade de tipos na subtraÃ§Ã£o!\n");
            $$ = ast_new_binop(MINUS, $1, $3);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_binop(MINUS, $1, $3);
            $$->expr_type = $1->expr_type;
        }
      }
    | expr TIMES expr     {
        if (!$1 || !$3 || $1->expr_type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Incopatibilidade de tipos na multiplicaÃ§Ã£o!\n");
            $$ = ast_new_binop(TIMES, $1, $3);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_binop(TIMES, $1, $3);
            $$->expr_type = $1->expr_type;
        }
      }
    | expr DIVIDE expr    {
        if (!$1 || !$3 || $1->expr_type != $3->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Incopatibilidade de tipos na divisÃ£o!\n");
            $$ = ast_new_binop(DIVIDE, $1, $3);
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$ = ast_new_binop(DIVIDE, $1, $3);
            $$->expr_type = $1->expr_type;
        }
      }
    | expr MOD expr {
        $$ = ast_new_binop(MOD, $1, $3);
        if (!$1 || !$3) {
            fprintf(stderr, "[ERRO SEMANTICO] Operandos invalidos para o modulo.\n");
            $$->expr_type = TYPE_UNKNOWN;
        } else if ($1->expr_type != TYPE_INT || $3->expr_type != TYPE_INT) {
            fprintf(stderr, "[ERRO SEMANTICO] O operador modulo (%%) exige operandos inteiros.\n");
            $$->expr_type = TYPE_UNKNOWN;
        } else if ($3->type == AST_NUM && $3->intval == 0) {
            fprintf(stderr, "[ERRO SEMANTICO] Divisao por zero no operador modulo (%%).\n");
            $$->expr_type = TYPE_UNKNOWN;
        } else {
            $$->expr_type = TYPE_INT;
        }
      }
    | expr LT expr { $$ = make_comparison(LT, $1, $3); }
    | expr GT expr { $$ = make_comparison(GT, $1, $3); }
    | expr LE expr { $$ = make_comparison(LE, $1, $3); }
    | expr GE expr { $$ = make_comparison(GE, $1, $3); }
    | expr EQ expr { $$ = make_comparison(EQ, $1, $3); }
    | expr NE expr { $$ = make_comparison(NE, $1, $3); }
    | LPAREN expr RPAREN  { $$ = $2; }
    | NUM                 { $$ = ast_new_num($1); }
    | CHAR_LITERAL        { $$ = ast_new_char($1); }
    | ID                  {
        Symbol *s = lookup_symbol($1, current_scope);
        if (!s) {
            if (lookup_function($1))
                fprintf(stderr, "[ERRO SEMANTICO] Funcao '%s' usada sem chamada.\n", $1);
            else
                fprintf(stderr, "[ERRO SEMANTICO] Variavel '%s' nao declarada!\n", $1);
            $$ = ast_new_id($1, TYPE_UNKNOWN);
        } else if (s->is_array) {
            fprintf(stderr, "[ERRO SEMANTICO] Vetor '%s' deve ser acessado com indice.\n", $1);
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
    | ID LPAREN args_list RPAREN {
        int valid_call = validate_function_call($1, $3);
        $$ = ast_new_func_call($1, $3);
        if (!valid_call && $$)
            $$->expr_type = TYPE_UNKNOWN;
        free($1);
      }
    ;

func_decl:
      type ID LPAREN { current_scope++; current_function_signature_valid = 1; } param_list RPAREN {
          current_function_return_type = $1;
          if (current_function_signature_valid)
              register_function($2, $1, $5);
      } LBRACE program RBRACE {
          close_scope(current_scope);
          current_scope--;
          if (($1 == TYPE_INT || $1 == TYPE_CHAR) && !always_returns($9)) {
              fprintf(stderr, "[ERRO SEMANTICO] Funcao '%s' pode terminar sem retornar um valor.\n", $2);
          }
          $$ = ast_new_func_decl($2, $1, $5, $9);
          current_function_return_type = TYPE_UNKNOWN;
          free($2);
      }
    ;

param_list:
      %empty                 { $$ = NULL; }
    | param                  { $$ = $1; }
    | param_list COMMA param { $$ = ast_new_param_list($1, $3); }
    ;

param:
      type ID {
          Symbol *s = NULL;
          if ($1 == TYPE_VOID) {
              fprintf(stderr, "[ERRO SEMANTICO] Parametro '%s' nao pode ter tipo void.\n", $2);
              current_function_signature_valid = 0;
          } else {
              s = insert_symbol($2, $1, 0, 0, current_scope);
          }
          if ($1 != TYPE_VOID && !s) {
              fprintf(stderr, "[ERRO SEMANTICO] Parametro '%s' duplicado no escopo %d.\n", $2, current_scope);
              current_function_signature_valid = 0;
          }
          $$ = ast_new_param($2, $1);
          free($2);
      }
    ;

args_list:
      %empty                 { $$ = NULL; }
    | expr                   { $$ = ast_new_args_list(NULL, $1); }
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
           fprintf(stderr, "[ERRO SEMANTICO] A condiÃ§Ã£o do if deve ser booleana ou inteira\n");
        }
        $$ = ast_new_if($3, $5);
      }
    | IF LPAREN expr RPAREN stmt ELSE stmt {
        if (!$3 || ($3->expr_type != TYPE_BOOL && $3->expr_type != TYPE_INT)) {
           fprintf(stderr, "[ERRO SEMANTICO] A condiÃ§Ã£o do if deve ser booleana ou inteira\n");
        }
        $$ = ast_new_if_else($3, $5, $7);
      }
    ;

while_stmt:
      WHILE LPAREN expr RPAREN stmt {
        if (!$3 || ($3->expr_type != TYPE_BOOL && $3->expr_type != TYPE_INT)) {
           fprintf(stderr, "[ERRO SEMANTICO] A condiÃ§Ã£o do while deve ser booleana ou inteira\n");
        }
        $$ = ast_new_while($3, $5);
      }
    ;

for_stmt:
      FOR LPAREN { current_scope++; } for_init for_condition SEMI for_update RPAREN stmt {
        close_scope(current_scope);
        current_scope--;
        if ($5 && $5->expr_type != TYPE_BOOL && $5->expr_type != TYPE_INT) {
            fprintf(stderr, "[ERRO SEMANTICO] Condicao do for deve ser booleana ou inteira.\n");
        }
        $$ = ast_new_for($4, $5, $7, $9);
      }
    ;

for_init:
      SEMI { $$ = NULL; }
    | decl_stmt { $$ = $1; }
    | assign_stmt { $$ = $1; }
    | expr SEMI { $$ = $1; }
    | type ID ASSIGN expr SEMI {
        Symbol *s = NULL;
        if ($1 == TYPE_VOID) {
            fprintf(stderr, "[ERRO SEMANTICO] Variavel do for nao pode ter tipo void.\n");
        } else {
            s = insert_symbol($2, $1, 0, 0, current_scope);
        }
        if (!s || !$4) {
            if ($1 != TYPE_VOID)
                fprintf(stderr, "[ERRO SEMANTICO] Declaracao inicial invalida para '%s' no for.\n", $2);
            $$ = NULL;
        } else if (s->type != $4->expr_type) {
            fprintf(stderr, "[ERRO SEMANTICO] Tipo incompativel na inicializacao de '%s' no for.\n", $2);
            $$ = NULL;
        } else {
            $$ = ast_new_seq(ast_new_decl($2, $1), ast_new_assign($2, $4));
        }
        free($2);
    }
    ;

for_condition:
      %empty { $$ = NULL; }
    | expr { $$ = $1; }
    ;

for_update:
      %empty { $$ = NULL; }
    | expr { $$ = $1; }
    | ID ASSIGN expr {
        $$ = make_for_assignment($1, $3);
        free($1);
    }
    | ID LBRACK expr RBRACK ASSIGN expr {
        Symbol *s = lookup_symbol($1, current_scope);
        int valid = s && s->is_array && $3 && $3->expr_type == TYPE_INT && $6 && s->type == $6->expr_type;
        if (!s || !s->is_array)
            fprintf(stderr, "[ERRO SEMANTICO] Alvo de vetor invalido no incremento do for.\n");
        if (!$3 || $3->expr_type != TYPE_INT)
            fprintf(stderr, "[ERRO SEMANTICO] Indice do vetor no for deve ser inteiro.\n");
        if (s && s->is_array && $6 && s->type != $6->expr_type)
            fprintf(stderr, "[ERRO SEMANTICO] Tipo incompativel no incremento do for.\n");
        $$ = valid ? ast_new_array_assign($1, $3, $6) : NULL;
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
