#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "symtab.h"

void test_insert_and_lookup() {
    symtab_init();

    // Inserção de x global (escopo 0) e variável global g
    Symbol *s = insert_symbol("x", TYPE_INT, 0, 0, 0);
    Symbol *g = insert_symbol("g", TYPE_CHAR, 0, 0, 0);
    assert(s != NULL);
    assert(g != NULL);
    assert(strcmp(s->name, "x") == 0);
    assert(s->scope == 0);

    // Busca no escopo 0
    Symbol *found = lookup_symbol("x", 0);
    assert(found != NULL);
    assert(found->type == TYPE_INT);
    assert(found->scope == 0);

    // Redeclaração no mesmo escopo deve falhar
    Symbol *dup = insert_symbol("x", TYPE_FLOAT, 0, 0, 0);
    assert(dup == NULL);

    // Shadowing: inserção de x no escopo 1 com outro tipo
    Symbol *shadow = insert_symbol("x", TYPE_FLOAT, 0, 0, 1);
    assert(shadow != NULL);
    assert(shadow->scope == 1);
    assert(shadow->type == TYPE_FLOAT);

    // Busca de x no escopo 1 deve retornar o x local (TYPE_FLOAT)
    Symbol *found_local = lookup_symbol("x", 1);
    assert(found_local != NULL);
    assert(found_local->scope == 1);
    assert(found_local->type == TYPE_FLOAT);

    // Busca de x no escopo 0 deve continuar retornando o x global (TYPE_INT)
    Symbol *found_global = lookup_symbol("x", 0);
    assert(found_global != NULL);
    assert(found_global->scope == 0);
    assert(found_global->type == TYPE_INT);

    // Busca de variável global 'g' a partir do escopo 1 deve encontrá-la no escopo 0
    Symbol *found_g = lookup_symbol("g", 1);
    assert(found_g != NULL);
    assert(found_g->scope == 0);

    // Fechamento do escopo 1: desativa o x local, voltando a enxergar o x global
    close_scope(1);
    Symbol *after_close = lookup_symbol("x", 1);
    assert(after_close != NULL);
    assert(after_close->scope == 0);
    assert(after_close->type == TYPE_INT);

    free_symtab();
}

void test_function_signatures() {
    symtab_init();

    DataType params[] = { TYPE_INT, TYPE_FLOAT };
    Symbol *fn = insert_function("calcula", TYPE_FLOAT, 2, params);
    assert(fn != NULL);
    assert(fn->is_function == 1);
    assert(fn->return_type == TYPE_FLOAT);
    assert(fn->param_count == 2);
    assert(fn->param_types[0] == TYPE_INT);
    assert(fn->param_types[1] == TYPE_FLOAT);

    Symbol *found_fn = lookup_function("calcula");
    assert(found_fn == fn);

    print_symtab();
    free_symtab();
}

int main() {
    printf("Rodando testes unitários da Tabela de Símbolos...\n");
    test_insert_and_lookup();
    test_function_signatures();
    printf("Todos os testes da Tabela de Símbolos passaram com sucesso!\n");
    return 0;
}