#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

static void print_indent(int level) {
    for (int i = 0; i < level; i++) printf("  ");
}

ASTNode* ast_new_num(int val) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_NUM;
    node->expr_type = TYPE_INT;
    node->intval = val;
    return node;
}

ASTNode* ast_new_id(const char *id, DataType type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ID;
    node->expr_type = type;
    node->id = strdup(id);
    return node;
}

ASTNode* ast_new_binop(int op, ASTNode *left, ASTNode *right) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_BINOP;
    node->expr_type = left ? left->expr_type : TYPE_INT;
    node->binop.op = op;
    node->binop.left = left;
    node->binop.right = right;
    return node;
}

ASTNode* ast_new_if(ASTNode *cond, ASTNode *if_branch) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->expr_type = TYPE_VOID;
    node->if_stmt.cond = cond;
    node->if_stmt.if_branch = if_branch;
    node->if_stmt.else_branch = NULL;
    return node;
}

ASTNode* ast_new_if_else(ASTNode *cond, ASTNode *if_branch, ASTNode *else_branch) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->expr_type = TYPE_VOID;
    node->if_stmt.cond = cond;
    node->if_stmt.if_branch = if_branch;
    node->if_stmt.else_branch = else_branch;
    return node;
}

ASTNode* ast_new_while(ASTNode *cond, ASTNode *body) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_WHILE;
    node->expr_type = TYPE_VOID;
    node->while_stmt.cond = cond;
    node->while_stmt.body = body;
    return node;
}

ASTNode* ast_new_for(ASTNode *init, ASTNode *cond, ASTNode *inc, ASTNode *body) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_FOR;
    node->expr_type = TYPE_VOID;
    node->for_stmt.init = init;
    node->for_stmt.cond = cond;
    node->for_stmt.inc = inc;
    node->for_stmt.body = body;
    return node;
}

ASTNode* ast_new_assign(const char *id, ASTNode *expr) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->expr_type = expr ? expr->expr_type : TYPE_UNKNOWN;
    node->assign.id = strdup(id);
    node->assign.expr = expr;
    node->assign.index_expr = NULL;
    return node;
}

ASTNode* ast_new_array_assign(const char *id, ASTNode *index_expr, ASTNode *expr) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->expr_type = expr ? expr->expr_type : TYPE_UNKNOWN;
    node->assign.id = strdup(id);
    node->assign.index_expr = index_expr;
    node->assign.expr = expr;
    return node;
}

ASTNode* ast_new_decl(const char *id, DataType data_type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_DECL;
    node->expr_type = data_type;
    node->decl.id = strdup(id);
    node->decl.data_type = data_type;
    node->decl.is_array = 0;
    node->decl.array_size = 0;
    return node;
}

ASTNode* ast_new_array_decl(const char *id, DataType data_type, int array_size) {
    ASTNode *node = ast_new_decl(id, data_type);
    if (node) {
        node->decl.is_array = 1;
        node->decl.array_size = array_size;
    }
    return node;
}

ASTNode* ast_new_array_access(const char *id, ASTNode *index_expr, DataType data_type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    if (!node) return NULL;
    node->type = AST_ARRAY_ACCESS;
    node->expr_type = data_type;
    node->array_access.id = strdup(id);
    node->array_access.index_expr = index_expr;
    return node;
}

ASTNode* ast_new_seq(ASTNode *stmt1, ASTNode *stmt2) {
    if (stmt1 == NULL) return stmt2;
    if (stmt2 == NULL) return stmt1;
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_SEQ;
    node->expr_type = TYPE_VOID;
    node->seq.stmt1 = stmt1;
    node->seq.stmt2 = stmt2;
    return node;
}

ASTNode* ast_new_func_decl(const char *id, DataType ret_type, ASTNode *params, ASTNode *body) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_FUNC_DECL;
    node->expr_type = ret_type;
    node->func_decl.id = strdup(id);
    node->func_decl.return_type = ret_type;
    node->func_decl.params = params;
    node->func_decl.body = body;
    return node;
}

ASTNode* ast_new_func_call(const char *id, ASTNode *args) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_FUNC_CALL;
    Symbol *fn = lookup_function(id); 
    node->expr_type = fn ? fn->return_type : TYPE_UNKNOWN; 
    node->func_call.id = strdup(id);
    node->func_call.args = args;
    return node;
}

ASTNode* ast_new_param(const char *id, DataType type) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_PARAM;
    node->expr_type = type;
    node->param.id = strdup(id);
    node->param.type = type;
    return node;
}

ASTNode* ast_new_param_list(ASTNode *param_list, ASTNode *param) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_PARAM_LIST;
    node->expr_type = TYPE_VOID;
    node->param_list.param = param;
    node->param_list.next = param_list;
    return node;
}

ASTNode* ast_new_args_list(ASTNode *args_list, ASTNode *expr) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    node->type = AST_ARGS_LIST;
    node->expr_type = TYPE_VOID;
    node->args_list.expr = expr;
    node->args_list.next = args_list;
    return node;
}

ASTNode* ast_new_return(ASTNode *expr) {
    ASTNode *node = (ASTNode*) malloc(sizeof(ASTNode));
    if (node != NULL) {
        node->type = AST_RETURN;
        node->expr_type = expr ? expr->expr_type : TYPE_VOID;
        node->return_stmt.expr = expr;
    }
    return node;
}

void ast_print(ASTNode *node, int level) {
    if (!node) return;

    if (node->type == AST_SEQ) {
        ast_print(node->seq.stmt1, level);
        ast_print(node->seq.stmt2, level);
        return;
    }
    if (node->type == AST_PARAM_LIST) {
        ast_print(node->param_list.next, level);
        ast_print(node->param_list.param, level);
        return;
    }
    if (node->type == AST_ARGS_LIST) {
        ast_print(node->args_list.next, level);
        ast_print(node->args_list.expr, level);
        return;
    }

    print_indent(level);

    switch (node->type) {
        case AST_NUM:  printf("NUM(%d)\n", node->intval); break;
        case AST_ID:   printf("ID(%s)\n", node->id); break;
        case AST_DECL:
            if (node->decl.is_array)
                printf("ARRAY_DECLARATION(%s, %d)\n", node->decl.id, node->decl.array_size);
            else
                printf("DECLARATION(%s)\n", node->decl.id);
            break;
        case AST_ARRAY_ACCESS:
            printf("ARRAY_ACCESS(%s)\n", node->array_access.id);
            ast_print(node->array_access.index_expr, level + 1);
            break;
        case AST_BINOP:
            printf("OP(%d)\n", node->binop.op);
            ast_print(node->binop.left, level + 1);
            ast_print(node->binop.right, level + 1);
            break;
        case AST_ASSIGN:
            if (node->assign.index_expr) {
                printf("ARRAY_ASSIGN(%s)\n", node->assign.id);
                ast_print(node->assign.index_expr, level + 1);
            } else {
                printf("ASSIGN(%s)\n", node->assign.id);
            }
            ast_print(node->assign.expr, level + 1);
            break;
        case AST_IF:
            printf("IF\n");
            ast_print(node->if_stmt.cond, level + 1);
            print_indent(level);
            printf("THEN:\n");
            ast_print(node->if_stmt.if_branch, level + 1);
            if (node->if_stmt.else_branch != NULL) {
                print_indent(level);
                printf("ELSE:\n");
                ast_print(node->if_stmt.else_branch, level + 1);
            }
            break;
        case AST_WHILE:
            printf("WHILE\n");
            print_indent(level + 1);
            printf("COND:\n");
            ast_print(node->while_stmt.cond, level + 2);
            print_indent(level + 1);
            printf("BODY:\n");
            ast_print(node->while_stmt.body, level + 2);
            break;
        case AST_FOR:
            printf("FOR\n");
            if (node->for_stmt.init) {
                print_indent(level + 1);
                printf("INIT:\n");
                ast_print(node->for_stmt.init, level + 2);
            }
            if (node->for_stmt.cond) {
                print_indent(level + 1);
                printf("COND:\n");
                ast_print(node->for_stmt.cond, level + 2);
            }
            if (node->for_stmt.inc) {
                print_indent(level + 1);
                printf("INC:\n");
                ast_print(node->for_stmt.inc, level + 2);
            }
            print_indent(level + 1);
            printf("BODY:\n");
            ast_print(node->for_stmt.body, level + 2);
            break;
        case AST_FUNC_DECL:
            printf("FUNC_DECL(%s)\n", node->func_decl.id);
            if (node->func_decl.params) {
                print_indent(level + 1);
                printf("PARAMS:\n");
                ast_print(node->func_decl.params, level + 2);
            }
            print_indent(level + 1);
            printf("BODY:\n");
            ast_print(node->func_decl.body, level + 2);
            break;
        case AST_FUNC_CALL:
            printf("FUNC_CALL(%s)\n", node->func_call.id);
            if (node->func_call.args) {
                print_indent(level + 1);
                printf("ARGS:\n");
                ast_print(node->func_call.args, level + 2);
            }
            break;
        case AST_PARAM:
            printf("PARAM(%s)\n", node->param.id);
            break;
        case AST_RETURN:
            printf("RETURN\n");
            ast_print(node->return_stmt.expr, level + 1);
            break;
        default:
            printf("Node Indefinido (%d)\n", node->type);
            break;
    }
}

void ast_free(ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case AST_ID:        free(node->id); break;
        case AST_BINOP:     ast_free(node->binop.left); ast_free(node->binop.right); break;
        case AST_ASSIGN:    free(node->assign.id); ast_free(node->assign.index_expr); ast_free(node->assign.expr); break;
        case AST_DECL:      free(node->decl.id); break;
        case AST_ARRAY_ACCESS:
            free(node->array_access.id);
            ast_free(node->array_access.index_expr);
            break;
        case AST_SEQ:       ast_free(node->seq.stmt1); ast_free(node->seq.stmt2); break;
        case AST_IF:
            ast_free(node->if_stmt.cond);
            ast_free(node->if_stmt.if_branch);
            ast_free(node->if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->while_stmt.cond);
            ast_free(node->while_stmt.body);
            break;
        case AST_FOR:
            ast_free(node->for_stmt.init);
            ast_free(node->for_stmt.cond);
            ast_free(node->for_stmt.inc);
            ast_free(node->for_stmt.body);
            break;
        case AST_RETURN:    ast_free(node->return_stmt.expr); break;
        case AST_FUNC_DECL:
            free(node->func_decl.id);
            ast_free(node->func_decl.params);
            ast_free(node->func_decl.body);
            break;
        case AST_FUNC_CALL:
            free(node->func_call.id);
            ast_free(node->func_call.args);
            break;
        case AST_PARAM:     free(node->param.id); break;
        case AST_PARAM_LIST:
            ast_free(node->param_list.param);
            ast_free(node->param_list.next);
            break;
        case AST_ARGS_LIST:
            ast_free(node->args_list.expr);
            ast_free(node->args_list.next);
            break;
        default: break;
    }
    free(node);
}
