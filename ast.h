#ifndef AST_H
#define AST_H
#include "symtab.h" // Necessário para acessar o enum DataType

typedef enum {
    AST_NUM, AST_ID, AST_BINOP, AST_ASSIGN,
    AST_DECL, AST_SEQ, AST_IF, AST_WHILE, AST_RETURN
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    DataType expr_type; // Para análise semântica (tipo da expressão)
    
    union {
        int intval;   // Para AST_NUM
        char *id;     // Para AST_ID e AST_DECL
        
        struct { 
            int op; // Token da operação (PLUS, MINUS...)
            struct ASTNode *left; 
            struct ASTNode *right; 
        } binop; // Para AST_BINOP
        
        struct { 
            char *id; 
            struct ASTNode *expr; 
        } assign; // Para AST_ASSIGN
        
        struct { 
            DataType data_type; 
            char *id; 
        } decl; // Para AST_DECL
        
        struct { 
            struct ASTNode *stmt1; 
            struct ASTNode *stmt2; 
        } seq; // Para lista de comandos (AST_SEQ)
    };
} ASTNode;

// Construtores
ASTNode* ast_new_num(int val);
ASTNode* ast_new_id(const char *id, DataType type);
ASTNode* ast_new_binop(int op, ASTNode *left, ASTNode *right);
ASTNode* ast_new_assign(const char *id, ASTNode *expr);
ASTNode* ast_new_decl(const char *id, DataType data_type);
ASTNode* ast_new_seq(ASTNode *stmt1, ASTNode *stmt2);

// Funções utilitárias
void ast_print(ASTNode *node, int level);
void ast_free(ASTNode *node);

#endif