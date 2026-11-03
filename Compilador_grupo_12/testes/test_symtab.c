#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../symtab.h"

void test_insert_and_lookup() {
    symtab_init();

    Symbol *s = insert_symbol("x", 1, 0, 0, 0);
    assert(s != NULL);
    assert(strcmp(s->name, "x") == 0);
    assert(s->scope == 0);

    Symbol *found = lookup_symbol("x", 0);
    assert(found != NULL);
    assert(strcmp(found->name, "x") == 0);

    Symbol *dup = insert_symbol("x", 2, 0, 0, 0);
    assert(dup == NULL);

    Symbol *shadow = insert_symbol("x", 1, 0, 0, 1);
    assert(shadow != NULL);
    assert(shadow->scope == 1);
}

int main() {
    printf("Rodando testes unitários da Tabela de Símbolos...\n");
    test_insert_and_lookup();
    printf("Todos os testes da Tabela de Símbolos passaram com sucesso!\n");
    return 0;
}
