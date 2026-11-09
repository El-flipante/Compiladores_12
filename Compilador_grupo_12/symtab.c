#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

static Symbol *symbol_table = NULL;

//
const char* datatype_to_str(DataType type) {
    switch (type) {
        case TYPE_INT:   return "int";
        case TYPE_CHAR:  return "char";
        case TYPE_BOOL:  return "bool";
        case TYPE_FLOAT: return "float";
        case TYPE_VOID:  return "void";
        default:         return "unknown";
    }
}

void symtab_init(void) {
    free_symtab();
    symbol_table = NULL;
}

// Verifica apenas no escopo exato (para detectar redeclaração no mesmo escopo)
Symbol* lookup_symbol_in_scope(const char *name, int scope) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (curr->is_active && curr->scope == scope && strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// Busca variável ativa respeitando a sombra (do escopo solicitado até o escopo global 0)
Symbol* lookup_symbol(const char *name, int scope) {
    Symbol *best = NULL;
    Symbol *curr = symbol_table;

    while (curr != NULL) {
        if (curr->is_active && strcmp(curr->name, name) == 0 && curr->scope <= scope) {
            if (best == NULL || curr->scope > best->scope) {
                best = curr;
            }
        }
        curr = curr->next;
    }
    return best;
}

// Busca o símbolo ativo mais interno independentemente do número de escopo
Symbol* lookup_symbol_global(const char *name) {
    Symbol *best = NULL;
    Symbol *curr = symbol_table;

    while (curr != NULL) {
        if (curr->is_active && strcmp(curr->name, name) == 0) {
            if (best == NULL || curr->scope > best->scope) {
                best = curr;
            }
        }
        curr = curr->next;
    }
    return best;
}

Symbol* insert_symbol(const char *name, DataType type, int is_array, int array_size, int scope) {
    // Verifica se já existe variável ativa com o mesmo nome no MESMO escopo
    if (lookup_symbol_in_scope(name, scope) != NULL) {
        return NULL; // Erro: Variável já declarada no mesmo escopo
    }

    Symbol *sym = (Symbol*) malloc(sizeof(Symbol));
    if (!sym) return NULL;

    sym->name = strdup(name);
    sym->type = type;
    sym->is_array = is_array;
    sym->array_size = array_size;
    sym->scope = scope;
    sym->is_active = 1;

    sym->is_function = 0;
    sym->return_type = TYPE_UNKNOWN;
    sym->param_count = 0;
    memset(sym->param_types, 0, sizeof(sym->param_types));

    sym->next = symbol_table;
    symbol_table = sym;

    return sym;
}

Symbol* insert_function(const char *name, DataType return_type, int param_count, const DataType *param_types) {
    if (lookup_symbol_in_scope(name, 0) != NULL) {
        return NULL; // Função já declarada no escopo global
    }
    if (param_count < 0 || param_count > MAX_PARAMS) {
        return NULL;
    }

    Symbol *sym = (Symbol*) malloc(sizeof(Symbol));
    if (!sym) return NULL;

    sym->name = strdup(name);
    sym->type = return_type;
    sym->is_array = 0;
    sym->array_size = 0;
    sym->scope = 0;
    sym->is_active = 1;

    sym->is_function = 1;
    sym->return_type = return_type;
    sym->param_count = param_count;
    for (int i = 0; i < param_count; i++) {
        sym->param_types[i] = param_types ? param_types[i] : TYPE_UNKNOWN;
    }

    sym->next = symbol_table;
    symbol_table = sym;

    return sym;
}

int add_function_param(Symbol *func_sym, DataType param_type) {
    if (!func_sym || !func_sym->is_function || func_sym->param_count >= MAX_PARAMS) {
        return 0;
    }
    func_sym->param_types[func_sym->param_count++] = param_type;
    return 1;
}

Symbol* lookup_function(const char *name) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (curr->is_active && curr->is_function && strcmp(curr->name, name) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// Desativa variáveis ao fechar um escopo (mantendo histórico na tabela)
void close_scope(int scope) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (curr->scope == scope && !curr->is_function) {
            curr->is_active = 0;
        }
        curr = curr->next;
    }
}

// Remove fisicamente variáveis de um escopo fechado
void clear_scope(int scope) {
    Symbol *curr = symbol_table;
    Symbol *prev = NULL;

    while (curr != NULL) {
        if (curr->scope == scope && !curr->is_function) {
            Symbol *rem = curr;
            if (prev == NULL) {
                symbol_table = curr->next;
                curr = symbol_table;
            } else {
                prev->next = curr->next;
                curr = curr->next;
            }
            free(rem->name);
            free(rem);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

void free_symtab(void) {
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        Symbol *next = curr->next;
        free(curr->name);
        free(curr);
        curr = next;
    }
    symbol_table = NULL;
}

void print_symtab(void) {
    printf("\n=== TABELA DE SÍMBOLOS ===\n");
    Symbol *curr = symbol_table;
    while (curr != NULL) {
        if (curr->is_function) {
            printf("[FUNC] Nome: %-10s | Retorno: %-6s (%d) | Escopo: %d | Ativo: %d | Params(%d): (",
                   curr->name, datatype_to_str(curr->return_type), curr->return_type,
                   curr->scope, curr->is_active, curr->param_count);
            for (int i = 0; i < curr->param_count; i++) {
                printf("%s%s", datatype_to_str(curr->param_types[i]),
                       (i < curr->param_count - 1) ? ", " : "");
            }
            printf(")\n");
        } else {
            printf("[VAR]  Nome: %-10s | Tipo: %-6s (%d) | Array: %d | Escopo: %d | Ativo: %d\n",
                   curr->name, datatype_to_str(curr->type), curr->type,
                   curr->is_array, curr->scope, curr->is_active);
        }
        curr = curr->next;
    }
    printf("===========================\n\n");
}
