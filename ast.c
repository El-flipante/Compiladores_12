#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

ASTNode* ast_new_num(int val) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_NUM;
    node->expr_type = TYPE_INT; // Números inteiros por padrão
    node->intval = val;
    return node;
}

ASTNode* ast_new_id(const char *id, DataType type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ID;
    node->expr_type = type; // O tipo vem da Tabela de Símbolos
    node->id = strdup(id);
    return node;
}

ASTNode* ast_new_binop(int op, ASTNode *left, ASTNode *right) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_BINOP;
    // Semântica: se os tipos forem diferentes, poderíamos reportar erro aqui.
    // Para simplificar, assumimos o tipo do lado esquerdo.
    node->expr_type = left ? left->expr_type : TYPE_INT; 
    node->binop.op = op;
    node->binop.left = left;
    node->binop.right = right;
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

void ast_print(ASTNode *node, int level) {
    if (!node) return;
    for (int i = 0; i < level; i++) printf("  "); // Indentação

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
        default: printf("Node Indefinido\n");
    }
}