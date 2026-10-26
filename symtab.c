#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

static Symbol *symbol_table = NULL;

void symtab_init(void) {
    symbol_table = NULL;
}

Symbol* insert_symbol(const char *name, DataType type, int is_array, int array_size, int scope) {
    // Verifica se já existe no mesmo escopo
    Symbol *existing = lookup_symbol(name, scope);
    if (existing != NULL) {
        return NULL; // Erro: Variável já declarada no mesmo escopo
    }

    Symbol *sym = (Symbol*) malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = type;
    sym->is_array = is_array;
    sym->array_size = array_size;
    sym->scope = scope;
    sym->next = symbol_table;
    symbol_table = sym;

    return sym;
}

Symbol* lookup_symbol(const char *name, int scope) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0 && curr->scope == scope) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

Symbol* lookup_symbol_global(const char *name) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void print_symtab(void) {
    printf("\n=== TABELA DE SÍMBOLOS ===\n");
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        printf("Nome: %-10s | Tipo: %d | Array: %d | Escopo: %d\n", 
               curr->name, curr->type, curr->is_array, curr->scope);
        curr = curr->next;
    }
    printf("===========================\n\n");
}