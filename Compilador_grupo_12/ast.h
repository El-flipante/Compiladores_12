#ifndef AST_H
#define AST_H
#include "symtab.h"

typedef enum {
    AST_NUM, AST_ID, AST_BINOP, AST_ASSIGN,
    AST_DECL, AST_SEQ, AST_IF, AST_WHILE, AST_FOR, AST_RETURN,
    AST_FUNC_DECL, AST_FUNC_CALL, AST_PARAM, AST_PARAM_LIST, AST_ARGS_LIST
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    DataType expr_type;

    union {
        int intval;
        char *id;

        struct {
            int op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binop;

        struct {
            char *id;
            struct ASTNode *index_expr; // ADICIONADO PARA SUPORTE A ARRAY
            struct ASTNode *expr;
        } assign;

        struct {
            DataType data_type;
            char *id;
        } decl;

        struct {
            struct ASTNode *stmt1;
            struct ASTNode *stmt2;
        } seq;

        struct {
            struct ASTNode *cond;
            struct ASTNode *if_branch;
            struct ASTNode *else_branch;
        } if_stmt;

        struct {
            struct ASTNode *cond;
            struct ASTNode *body;
        } while_stmt;

        struct {
            struct ASTNode *init;
            struct ASTNode *cond;
            struct ASTNode *inc;
            struct ASTNode *body;
        } for_stmt;

        struct {
            struct ASTNode *expr;
        } return_stmt;

        struct {
            char *id;
            DataType return_type;
            struct ASTNode *params;
            struct ASTNode *body;
        } func_decl;

        struct {
            char *id;
            struct ASTNode *args;
        } func_call;

        struct {
            char *id;
            DataType type;
        } param;

        struct {
            struct ASTNode *param;
            struct ASTNode *next;
        } param_list;

        struct {
            struct ASTNode *expr;
            struct ASTNode *next;
        } args_list;
    };
} ASTNode;

ASTNode* ast_new_num(int val);
ASTNode* ast_new_id(const char *id, DataType type);
ASTNode* ast_new_binop(int op, ASTNode *left, ASTNode *right);
ASTNode* ast_new_assign(const char *id, ASTNode *expr);
ASTNode* ast_new_array_assign(const char *id, ASTNode *index_expr, ASTNode *expr); // NOVO
ASTNode* ast_new_decl(const char *id, DataType data_type);
ASTNode* ast_new_seq(ASTNode *stmt1, ASTNode *stmt2);
ASTNode* ast_new_if(ASTNode *cond, ASTNode *if_branch);
ASTNode* ast_new_if_else(ASTNode *cond, ASTNode *if_branch, ASTNode *else_branch);
ASTNode* ast_new_while(ASTNode *cond, ASTNode *body);
ASTNode* ast_new_for(ASTNode *init, ASTNode *cond, ASTNode *inc, ASTNode *body);
ASTNode* ast_new_return(ASTNode *expr);
ASTNode* ast_new_func_decl(const char *id, DataType ret_type, ASTNode *params, ASTNode *body);
ASTNode* ast_new_func_call(const char *id, ASTNode *args);
ASTNode* ast_new_param(const char *id, DataType type);
ASTNode* ast_new_param_list(ASTNode *param_list, ASTNode *param);
ASTNode* ast_new_args_list(ASTNode *args_list, ASTNode *expr);

void ast_print(ASTNode *node, int level);
void ast_free(ASTNode *node);

#endif