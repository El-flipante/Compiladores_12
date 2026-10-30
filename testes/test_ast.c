#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../ast.h"

void test_ast_creation() {

    ASTNode *num_node = ast_new_num(42);
    assert(num_node != NULL);
    assert(num_node->type == AST_NUM);
    assert(num_node->intval == 42);

    ASTNode *num_left = ast_new_num(10);
    ASTNode *num_right = ast_new_num(20);
    ASTNode *binop = ast_new_binop(1 /* PLUS hipotético */, num_left, num_right);

    assert(binop != NULL);
    assert(binop->type == AST_BINOP);
    assert(binop->binop.left->intval == 10);
    assert(binop->binop.right->intval == 20);
}

int main() {
    printf("Rodando testes unitários da AST...\n");
    test_ast_creation();
    printf("Todos os testes da AST passaram com sucesso!\n");
    return 0;
}
