#ifndef SYMTAB_H
#define SYMTAB_H

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
    struct Symbol *next;
} Symbol;

// Funções da Tabela de Símbolos
void symtab_init(void);
Symbol* insert_symbol(const char *name, DataType type, int is_array, int array_size, int scope);
Symbol* lookup_symbol(const char *name, int scope);
Symbol* lookup_symbol_global(const char *name);
void print_symtab(void);
void free_symtab(void);

#endif