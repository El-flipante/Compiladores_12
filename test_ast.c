#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../ast.h"

int main(void) {
    printf("Iniciando testes da Árvore Sintática Abstrata (AST)...\n\n");

    // 1. Teste de bloco IF / ELSE: if (x > 0) { y = 1; } else { y = 2; }
    ASTNode *if_cond = ast_new_binop('>', ast_new_id("x", TYPE_INT), ast_new_num(0));
    ASTNode *then_branch = ast_new_assign("y", ast_new_num(1));
    ASTNode *else_branch = ast_new_assign("y", ast_new_num(2));
    ASTNode *if_else_node = ast_new_if_else(if_cond, then_branch, else_branch);

    assert(if_else_node != NULL);
    assert(if_else_node->type == AST_IF);
    assert(if_else_node->if_stmt.cond == if_cond);
    assert(if_else_node->if_stmt.if_branch == then_branch);
    assert(if_else_node->if_stmt.else_branch == else_branch);

    printf("=== Teste 1: AST IF/ELSE ===\n");
    ast_print(if_else_node, 0);

    // 2. Teste de laço WHILE: while (i < 10) { i = i + 1; }
    ASTNode *while_cond = ast_new_binop('<', ast_new_id("i", TYPE_INT), ast_new_num(10));
    ASTNode *while_body = ast_new_assign("i", ast_new_binop('+', ast_new_id("i", TYPE_INT), ast_new_num(1)));
    ASTNode *while_node = ast_new_while(while_cond, while_body);

    assert(while_node != NULL);
    assert(while_node->type == AST_WHILE);
    assert(while_node->while_stmt.cond == while_cond);
    assert(while_node->while_stmt.body == while_body);

    printf("\n=== Teste 2: AST WHILE ===\n");
    ast_print(while_node, 0);

    // 3. Teste de laço FOR: for (i = 0; i < 5; i = i + 1) { total = total + i; }
    ASTNode *for_init = ast_new_assign("i", ast_new_num(0));
    ASTNode *for_cond = ast_new_binop('<', ast_new_id("i", TYPE_INT), ast_new_num(5));
    ASTNode *for_inc  = ast_new_assign("i", ast_new_binop('+', ast_new_id("i", TYPE_INT), ast_new_num(1)));
    ASTNode *for_body = ast_new_assign("total", ast_new_binop('+', ast_new_id("total", TYPE_INT), ast_new_id("i", TYPE_INT)));
    ASTNode *for_node = ast_new_for(for_init, for_cond, for_inc, for_body);

    assert(for_node != NULL);
    assert(for_node->type == AST_FOR);
    assert(for_node->for_stmt.init == for_init);
    assert(for_node->for_stmt.cond == for_cond);
    assert(for_node->for_stmt.inc == for_inc);
    assert(for_node->for_stmt.body == for_body);

    printf("\n=== Teste 3: AST FOR ===\n");
    ast_print(for_node, 0);

    // 4. Teste de Declaração e Chamada de Função:
    // int soma(int a, int b) { return a + b; }
    // res = soma(10, 20);
    ASTNode *p1 = ast_new_param("a", TYPE_INT);
    ASTNode *p2 = ast_new_param("b", TYPE_INT);
    ASTNode *params = ast_new_param_list(p1, p2);
    ASTNode *ret_stmt = ast_new_return(ast_new_binop('+', ast_new_id("a", TYPE_INT), ast_new_id("b", TYPE_INT)));
    ASTNode *func_decl = ast_new_func_decl("soma", TYPE_INT, params, ret_stmt);

    ASTNode *args = ast_new_args_list(ast_new_num(10), ast_new_num(20));
    ASTNode *func_call = ast_new_func_call("soma", args);
    ASTNode *call_assign = ast_new_assign("res", func_call);

    ASTNode *prog = ast_new_seq(func_decl, call_assign);

    assert(func_decl->type == AST_FUNC_DECL);
    assert(strcmp(func_decl->func_decl.id, "soma") == 0);
    assert(func_call->type == AST_FUNC_CALL);

    printf("\n=== Teste 4: AST FUNC_DECL e FUNC_CALL ===\n");
    ast_print(prog, 0);

    // Liberação de memória
    ast_free(if_else_node);
    ast_free(while_node);
    ast_free(for_node);
    ast_free(prog);

    printf("\nTodos os testes da AST passaram com sucesso!\n");
    return 0;
}