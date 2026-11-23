#ifndef IR_H
#define IR_H

#include <stdio.h>
#include "symtab.h"

struct ASTNode;

/*
 * Codigo intermediario: codigo de tres enderecos (TAC) em forma de quadruplas.
 *
 * Cada instrucao tem a forma  res = arg1 op arg2  (campos nao usados ficam em
 * OPD_NONE). Todos os valores sao palavras de 32 bits; o tipo so e registrado
 * onde o gerador de codigo final precisa dele (declaracoes, elementos de vetor,
 * parametros e retorno de funcoes).
 */

typedef enum {
    OPD_NONE,
    OPD_TEMP,    // temporario t<N> (a numeracao reinicia a cada funcao)
    OPD_VAR,     // variavel, parametro ou vetor (nome unico dentro da funcao)
    OPD_CONST,   // constante inteira (caracteres viram o seu codigo)
    OPD_LABEL,   // rotulo L<N> (numeracao unica no programa)
    OPD_FUNC     // nome de funcao
} OperandKind;

typedef struct {
    OperandKind kind;
    int value;       // numero do temporario/rotulo ou valor da constante
    char *name;      // nome da variavel/funcao
} Operand;

typedef enum {
    /* estrutura de funcoes */
    IR_FUNC_BEGIN,    // res = funcao, arg2 = CONST nº de parametros, type = tipo de retorno
    IR_FORMAL,        // res = parametro, type = tipo (um por parametro, em ordem)
    IR_FUNC_END,

    /* declaracoes */
    IR_GLOBAL,        // res = variavel global, type
    IR_GLOBAL_ARRAY,  // res = vetor global, arg2 = CONST tamanho, type = tipo do elemento
    IR_GLOBAL_INIT,   // res = vetor global, arg2 = CONST indice, arg1 = CONST valor
    IR_DECL,          // res = variavel local, type
    IR_DECL_ARRAY,    // res = vetor local, arg2 = CONST tamanho, type = tipo do elemento

    /* aritmeticas: res = arg1 op arg2 */
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    /* relacionais: res = (arg1 op arg2) ? 1 : 0 */
    IR_LT, IR_GT, IR_LE, IR_GE, IR_EQ, IR_NE,
    IR_NEG,           // res = -arg1

    /* dados */
    IR_COPY,          // res = arg1
    IR_ARRAY_LOAD,    // res = arg1[arg2]   (arg1 = vetor, type = tipo do elemento)
    IR_ARRAY_STORE,   // res[arg2] = arg1   (res = vetor, type = tipo do elemento)

    /* fluxo de controle */
    IR_LABEL,         // res = rotulo
    IR_GOTO,          // res = rotulo
    IR_IF_FALSE,      // se arg1 == 0 desvia para res

    /* funcoes */
    IR_PARAM,         // empilha o argumento arg1 (um por argumento, da esquerda p/ direita)
    IR_CALL,          // [res =] call arg1 (FUNC), arg2 = CONST nº de argumentos
    IR_RETURN,        // return [arg1]

    IR_COMMENT        // somente informativo (text)
} IROp;

typedef struct IRQuad {
    IROp op;
    Operand arg1, arg2, res;
    DataType type;
    char *text;               // somente para IR_COMMENT
    struct IRQuad *next;
} IRQuad;

typedef struct {
    IRQuad *head, *tail;
    int count;
    int errors;               // erros internos da geracao (0 = sucesso)
} IRProgram;

// Percorre a AST (ja validada semanticamente) e produz o codigo de tres enderecos.
IRProgram *ir_generate(struct ASTNode *root);
void ir_print(const IRProgram *prog, FILE *out);
void ir_free(IRProgram *prog);

#endif
