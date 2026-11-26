#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stddef.h>
#include "ir.h"

/*
 * Codigo final: Assembly RISC-V de 32 bits (RV32IM, ABI ilp32), no formato do
 * GNU as, gerado a partir do codigo de tres enderecos.
 *
 * O texto fica guardado em memoria, e so e impresso se a geracao terminar sem
 * erros (mesmo fluxo de IRProgram: gera, verifica 'errors', imprime, libera).
 */

typedef struct {
    char *text;               // Assembly completo, terminado em '\0'
    size_t length;
    int errors;               // erros internos da geracao (0 = sucesso)
} AsmProgram;

// Traduz o codigo intermediario (ja gerado sem erros) para Assembly RISC-V.
AsmProgram *codegen_generate(const IRProgram *ir);
void codegen_print(const AsmProgram *code, FILE *out);
void codegen_free(AsmProgram *code);

#endif
