#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

ASTNode* ast_new_num(int val) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_NUM;
    node->expr_type = TYPE_INT; // Numeros inteiros por padrao
    node->intval = val;
    return node;
}

ASTNode* ast_new_id(const char *id, DataType type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ID;
    node->expr_type = type; // O tipo vem da Tabela de Simbolos
    node->id = strdup(id);
    return node;
}

ASTNode* ast_new_binop(int op, ASTNode *left, ASTNode *right) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_BINOP;
    // Semantica: se os tipos forem diferentes, poderiamos reportar erro aqui.
    // Para simplificar, assumimos o tipo do lado esquerdo.
    node->expr_type = left ? left->expr_type : TYPE_INT; 
    node->binop.op = op;
    node->binop.left = left;
    node->binop.right = right;
    return node;
}

ASTNode* ast_new_if(ASTNode *cond, ASTNode *if_branch) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->if_stmt.cond = cond;
    node->if_stmt.if_branch = if_branch;
    node->if_stmt.else_branch = NULL;
    return node;
}

ASTNode* ast_new_if_else(ASTNode *cond, ASTNode *if_branch, ASTNode *else_branch) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->if_stmt.cond = cond;
    node->if_stmt.if_branch = if_branch;
    node->if_stmt.else_branch = else_branch;
    return node;
}

ASTNode* ast_new_assign(const char *id, ASTNode *expr) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->expr_type = expr->expr_type;
    node->assign.id = strdup(id);
    node->assign.expr = expr;
    return node;
}

ASTNode* ast_new_decl(const char *id, DataType data_type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_DECL;
    node->expr_type = data_type;
    node->decl.id = strdup(id);
    node->decl.data_type = data_type;
    return node;
}

ASTNode* ast_new_seq(ASTNode *stmt1, ASTNode *stmt2) {
    if (stmt1 == NULL) return stmt2;
    if (stmt2 == NULL) return stmt1;
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_SEQ;
    node->seq.stmt1 = stmt1;
    node->seq.stmt2 = stmt2;
    return node;
}

ASTNode* ast_new_func_decl(const char *id, DataType ret_type, ASTNode *params, ASTNode *body) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_FUNC_DECL;
    node->func_decl.id = strdup(id);
    node->func_decl.return_type = ret_type;
    node->func_decl.params = params;
    node->func_decl.body = body;
    return node;
}

ASTNode* ast_new_func_call(const char *id, ASTNode *args) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_FUNC_CALL;
    node->func_call.id = strdup(id);
    node->func_call.args = args;
    return node;
}

ASTNode* ast_new_param(const char *id, DataType type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_PARAM;
    node->param.id = strdup(id);
    node->param.type = type;
    return node;
}

ASTNode* ast_new_param_list(ASTNode *param_list, ASTNode *param) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_PARAM_LIST;
    node->param_list.param = param;
    node->param_list.next = param_list;
    return node;
}

ASTNode* ast_new_args_list(ASTNode *args_list, ASTNode *expr) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ARGS_LIST;
    node->args_list.expr = expr;
    node->args_list.next = args_list;
    return node;
}

// IMPRESSAO DA ARVORE

void ast_print(ASTNode *node, int level) {
    if (!node) return;
    for (int i = 0; i < level; i++) printf("  "); // Indentacao

    switch (node->type) {
        case AST_NUM:    printf("NUM(%d)\n", node->intval); break;
        case AST_ID:     printf("ID(%s)\n", node->id); break;
        case AST_DECL:   printf("DECLARATION(%s)\n", node->decl.id); break;
        case AST_BINOP:
            printf("OP(%d)\n", node->binop.op);
            ast_print(node->binop.left, level + 1);
            ast_print(node->binop.right, level + 1);
            break;
        case AST_ASSIGN:
            printf("ASSIGN(%s)\n", node->assign.id);
            ast_print(node->assign.expr, level + 1);
            break;
        case AST_SEQ:
            ast_print(node->seq.stmt1, level);
            ast_print(node->seq.stmt2, level);
            break;
        case AST_IF:
            printf("IF\n");
            ast_print(node->if_stmt.cond, level + 1);
            for(int i=0; i<level; i++) printf("  ");
            printf("THEN:\n");
            ast_print(node->if_stmt.if_branch, level + 1);
            if (node->if_stmt.else_branch != NULL) {
                for(int i=0; i<level; i++) printf("  ");
                printf("ELSE:\n");
                ast_print(node->if_stmt.else_branch, level + 1);
            }
            break;

        case AST_FUNC_DECL:
            printf("FUNC_DECL(%s)\n", node->func_decl.id);
            if (node->func_decl.params) {
                for(int i=0; i<level+1; i++) printf("  ");
                printf("PARAMS:\n");
                ast_print(node->func_decl.params, level + 2);
            }
            for(int i=0; i<level+1; i++) printf("  ");
            printf("BODY:\n");
            ast_print(node->func_decl.body, level + 2);
            break;
        case AST_FUNC_CALL:
            printf("FUNC_CALL(%s)\n", node->func_call.id);
            if (node->func_call.args) {
                for(int i=0; i<level+1; i++) printf("  ");
                printf("ARGS:\n");
                ast_print(node->func_call.args, level + 2);
            }
            break;
        case AST_PARAM:
            printf("PARAM(%s)\n", node->param.id);
            break;
        case AST_PARAM_LIST:
            ast_print(node->param_list.next, level);
            ast_print(node->param_list.param, level);
            break;
        case AST_ARGS_LIST:
            ast_print(node->args_list.next, level);
            ast_print(node->args_list.expr, level);
            break;

        default: printf("Node Indefinido (%d)\n", node->type);
    }
}
