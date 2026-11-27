#ifndef SYMTAB_H
#define SYMTAB_H

#define MAX_PARAMS 12

typedef enum {
    TYPE_INT,
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_FLOAT,
    TYPE_VOID,
    TYPE_UNKNOWN
} DataType;

typedef struct Symbol {
    char *name;
    DataType type;
    int is_array;
    int array_size;
    int scope;
    int is_active;        // 1 = ativo no escopo, 0 = desativado
    int is_function;      // 1 = função, 0 = variável/parâmetro
    DataType return_type; // Tipo de retorno da função
    int param_count;      // Número de parâmetros da função
    DataType param_types[MAX_PARAMS];  //Tipos de parâmetros
    struct Symbol *next;
} Symbol;

// Funções da Tabela de Símbolos
void symtab_init(void);
void free_symtab(void);
Symbol* insert_symbol(const char *name, DataType type, int is_array, int array_size, int scope);
Symbol* lookup_symbol_in_scope(const char *name, int scope);
Symbol* lookup_symbol(const char *name, int scope);
Symbol* lookup_symbol_global(const char *name);
void close_scope(int scope);
void clear_scope(int scope);
int add_function_param(Symbol *func_sym, DataType param_type);

Symbol* insert_function(const char *name, DataType return_type, int param_count, const DataType *param_types);
Symbol* lookup_function(const char *name);

const char* datatype_to_str(DataType type);
void print_symtab(void);

#endif