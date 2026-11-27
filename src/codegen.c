#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ir.h"
#include "codegen.h"

/*
 * Gerador de codigo final: traduz o codigo de tres enderecos (IRProgram) para
 * Assembly RISC-V de 32 bits (RV32IM, ABI ilp32), no formato do GNU as.
 *
 * Estrategia (sem alocacao de registradores):
 *   - variaveis, parametros, vetores locais e temporarios do codigo
 *     intermediario ficam em posicoes fixas do quadro de pilha da funcao,
 *     enderecadas a partir de s0 (frame pointer);
 *   - cada quadrupla carrega os operandos em registradores de trabalho,
 *     calcula e grava o resultado de volta no quadro;
 *   - chamadas seguem a convencao padrao: argumentos em a0-a7 (os demais na
 *     pilha), resultado em a0, retorno por ra.
 *
 * Registradores de trabalho:
 *   t0, t1  operandos e resultado        t2  endereco de elemento de vetor
 *   t6      endereco auxiliar em escritas (globais e deslocamentos grandes)
 *
 * Tipos: int/bool ocupam uma palavra (lw/sw); char e um byte com sinal (lb/sb)
 * em variaveis e vetores, o que trunca o valor a cada escrita.
 *
 * Quadro de uma funcao (a pilha cresce para enderecos menores):
 *
 *      | argumentos 9, 10, ...  |  s0 + 0, s0 + 4, ... (empilhados pelo chamador)
 *      +------------------------+  <- s0 (valor de sp na entrada da funcao)
 *      | ra                     |  s0 - 4
 *      | s0 do chamador         |  s0 - 8
 *      | parametros, variaveis, |
 *      | vetores e temporarios  |
 *      +------------------------+
 *      | argumentos de saida    |  <- sp (chamadas com mais de 8 argumentos)
 *      +------------------------+
 */

#define MAX_REG_ARGS 8     // argumentos passados em a0-a7

/* ------------------------------------------------------------------ */
/* Estado da geracao                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    char *name;       // nome no codigo intermediario
    int offset;       // locais: deslocamento em relacao a s0
    int is_array;
    int size;         // numero de elementos (vetores)
    DataType type;    // tipo da variavel ou do elemento
} VarSlot;

typedef struct {
    VarSlot *v;
    int n, cap;
} VarTable;

static FILE *out;              // fluxo em memoria que recebe o Assembly
static int errors;

static VarTable globals;       // variaveis globais do programa
static VarTable locals;        // parametros e variaveis da funcao corrente

static const char *cur_name;   // funcao corrente
static DataType cur_ret_type;
static int frame_size;         // tamanho do quadro (multiplo de 16)
static int temps_start;        // bytes do quadro ocupados antes dos temporarios
static int temps_max;          // maior temporario usado na funcao
static int param_index;        // 'param' ja carregados para a proxima chamada

/* ------------------------------------------------------------------ */
/* Utilitarios                                                         */
/* ------------------------------------------------------------------ */

static char *dup_str(const char *s) {
    char *c = strdup(s);
    if (!c) { fprintf(stderr, "[ERRO CODIGO FINAL] memoria insuficiente.\n"); exit(1); }
    return c;
}

static void cg_error(const char *msg, const char *detail) {
    if (detail) fprintf(stderr, "[ERRO CODIGO FINAL] %s '%s'.\n", msg, detail);
    else        fprintf(stderr, "[ERRO CODIGO FINAL] %s.\n", msg);
    errors++;
}

// Escreve uma instrucao (ou diretiva) indentada.
static void emit(const char *fmt, ...) {
    va_list ap;
    fputs("    ", out);
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fputc('\n', out);
}

static int round_up(int v, int m) { return (v + m - 1) / m * m; }

static int elem_size(DataType t)       { return t == TYPE_CHAR ? 1 : 4; }
static const char *load_op(DataType t)  { return t == TYPE_CHAR ? "lb" : "lw"; }
static const char *store_op(DataType t) { return t == TYPE_CHAR ? "sb" : "sw"; }

/* ------------------------------------------------------------------ */
/* Tabelas de variaveis                                                */
/* ------------------------------------------------------------------ */

static void vt_add(VarTable *t, const char *name, int offset, int is_array,
                   int size, DataType type) {
    if (t->n == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 16;
        t->v = (VarSlot *) realloc(t->v, (size_t) t->cap * sizeof(VarSlot));
        if (!t->v) { fprintf(stderr, "[ERRO CODIGO FINAL] memoria insuficiente.\n"); exit(1); }
    }
    VarSlot *s = &t->v[t->n++];
    s->name = dup_str(name);
    s->offset = offset;
    s->is_array = is_array;
    s->size = size;
    s->type = type;
}

static VarSlot *vt_find(const VarTable *t, const char *name) {
    for (int i = 0; i < t->n; i++)
        if (strcmp(t->v[i].name, name) == 0) return &t->v[i];
    return NULL;
}

static void vt_clear(VarTable *t) {
    for (int i = 0; i < t->n; i++) free(t->v[i].name);
    t->n = 0;
}

static void vt_free(VarTable *t) {
    vt_clear(t);
    free(t->v);
    t->v = NULL;
    t->cap = 0;
}

// Procura primeiro nas variaveis da funcao e depois nas globais.
static VarSlot *lookup_var(const char *name, int *is_local) {
    VarSlot *v = vt_find(&locals, name);
    *is_local = (v != NULL);
    if (!v) v = vt_find(&globals, name);
    if (!v) cg_error("variavel nao declarada", name);
    return v;
}

/* ------------------------------------------------------------------ */
/* Acesso a memoria                                                    */
/* ------------------------------------------------------------------ */

// O campo de deslocamento das instrucoes de memoria tem 12 bits com sinal.
static int fits_imm12(int v) { return v >= -2048 && v <= 2047; }

// reg <- mem[s0 + off]
static void load_local(const char *op, const char *reg, int off) {
    if (fits_imm12(off)) {
        emit("%s %s, %d(s0)", op, reg, off);
    } else {
        emit("li %s, %d", reg, off);
        emit("add %s, %s, s0", reg, reg);
        emit("%s %s, 0(%s)", op, reg, reg);
    }
}

// mem[s0 + off] <- reg
static void store_local(const char *op, const char *reg, int off) {
    if (fits_imm12(off)) {
        emit("%s %s, %d(s0)", op, reg, off);
    } else {
        emit("li t6, %d", off);
        emit("add t6, t6, s0");
        emit("%s %s, 0(t6)", op, reg);
    }
}

// reg <- endereco da variavel ou do vetor
static void addr_of(const char *reg, const char *name) {
    int is_local;
    VarSlot *v = lookup_var(name, &is_local);
    if (!v) return;
    if (!is_local) {
        emit("la %s, %s", reg, name);
    } else if (fits_imm12(v->offset)) {
        emit("addi %s, s0, %d", reg, v->offset);
    } else {
        emit("li %s, %d", reg, v->offset);
        emit("add %s, %s, s0", reg, reg);
    }
}

static void load_var(const char *reg, const char *name) {
    int is_local;
    VarSlot *v = lookup_var(name, &is_local);
    if (!v) { emit("li %s, 0", reg); return; }
    if (is_local) {
        load_local(load_op(v->type), reg, v->offset);
    } else {
        emit("la %s, %s", reg, name);
        emit("%s %s, 0(%s)", load_op(v->type), reg, reg);
    }
}

static void store_var(const char *reg, const char *name) {
    int is_local;
    VarSlot *v = lookup_var(name, &is_local);
    if (!v) return;
    if (is_local) {
        store_local(store_op(v->type), reg, v->offset);
    } else {
        emit("la t6, %s", name);
        emit("%s %s, 0(t6)", store_op(v->type), reg);
    }
}

// Cada temporario t<N> ocupa uma palavra, logo abaixo das variaveis da funcao.
static int temp_offset(int n) {
    if (n < 1 || n > temps_max) cg_error("temporario inexistente na funcao", cur_name);
    return -(temps_start + 4 * n);
}

static void load_operand(const char *reg, const Operand *o) {
    switch (o->kind) {
        case OPD_CONST: emit("li %s, %d", reg, o->value); break;
        case OPD_TEMP:  load_local("lw", reg, temp_offset(o->value)); break;
        case OPD_VAR:   load_var(reg, o->name); break;
        default:
            cg_error("operando invalido em instrucao do codigo intermediario", NULL);
            emit("li %s, 0", reg);
            break;
    }
}

static void store_operand(const char *reg, const Operand *o) {
    switch (o->kind) {
        case OPD_TEMP: store_local("sw", reg, temp_offset(o->value)); break;
        case OPD_VAR:  store_var(reg, o->name); break;
        case OPD_NONE: break;
        default:
            cg_error("destino invalido em instrucao do codigo intermediario", NULL);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Instrucoes                                                          */
/* ------------------------------------------------------------------ */

static void gen_binary(const IRQuad *q) {
    load_operand("t0", &q->arg1);
    load_operand("t1", &q->arg2);
    switch (q->op) {
        case IR_ADD: emit("add t0, t0, t1"); break;
        case IR_SUB: emit("sub t0, t0, t1"); break;
        case IR_MUL: emit("mul t0, t0, t1"); break;
        case IR_DIV: emit("div t0, t0, t1"); break;
        case IR_MOD: emit("rem t0, t0, t1"); break;
        case IR_LT:  emit("slt t0, t0, t1"); break;
        case IR_GT:  emit("slt t0, t1, t0"); break;
        case IR_LE:  emit("slt t0, t1, t0"); emit("xori t0, t0, 1"); break;   // !(a > b)
        case IR_GE:  emit("slt t0, t0, t1"); emit("xori t0, t0, 1"); break;   // !(a < b)
        case IR_EQ:  emit("xor t0, t0, t1"); emit("seqz t0, t0"); break;
        case IR_NE:  emit("xor t0, t0, t1"); emit("snez t0, t0"); break;
        default: break;
    }
    store_operand("t0", &q->res);
}

// res = arg1[arg2]  ou  res[arg2] = arg1  (o tipo da quadrupla e o do elemento)
static void gen_array_access(const IRQuad *q) {
    int is_load = (q->op == IR_ARRAY_LOAD);
    const Operand *array = is_load ? &q->arg1 : &q->res;

    if (!is_load) load_operand("t0", &q->arg1);          // valor a gravar
    load_operand("t1", &q->arg2);                         // indice
    if (elem_size(q->type) == 4) emit("slli t1, t1, 2");
    addr_of("t2", array->name);
    emit("add t2, t2, t1");
    if (is_load) {
        emit("%s t0, 0(t2)", load_op(q->type));
        store_operand("t0", &q->res);
    } else {
        emit("%s t0, 0(t2)", store_op(q->type));
    }
}

static void gen_quad(const IRQuad *q) {
    switch (q->op) {
        case IR_FORMAL:
        case IR_DECL:
        case IR_DECL_ARRAY:
            break;   // o espaco ja foi reservado no quadro; nada a executar

        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
        case IR_LT: case IR_GT: case IR_LE: case IR_GE: case IR_EQ: case IR_NE:
            gen_binary(q);
            break;

        case IR_NEG:
            load_operand("t0", &q->arg1);
            emit("neg t0, t0");
            store_operand("t0", &q->res);
            break;

        case IR_COPY:
            load_operand("t0", &q->arg1);
            store_operand("t0", &q->res);
            break;

        case IR_ARRAY_LOAD:
        case IR_ARRAY_STORE:
            gen_array_access(q);
            break;

        case IR_LABEL:
            fprintf(out, ".L%d:\n", q->res.value);
            break;

        case IR_GOTO:
            emit("j .L%d", q->res.value);
            break;

        case IR_IF_FALSE:
            load_operand("t0", &q->arg1);
            emit("beqz t0, .L%d", q->res.value);
            break;

        case IR_PARAM:
            if (param_index < MAX_REG_ARGS) {
                char reg[16];
                snprintf(reg, sizeof reg, "a%d", param_index);
                load_operand(reg, &q->arg1);
            } else {
                load_operand("t0", &q->arg1);
                emit("sw t0, %d(sp)", (param_index - MAX_REG_ARGS) * 4);
            }
            param_index++;
            break;

        case IR_CALL:
            if (param_index != q->arg2.value)
                cg_error("numero de 'param' diferente do numero de argumentos da chamada", q->arg1.name);
            emit("jal ra, %s", q->arg1.name);
            param_index = 0;
            store_operand("a0", &q->res);
            break;

        case IR_RETURN:
            if (q->arg1.kind != OPD_NONE) {
                load_operand("a0", &q->arg1);
                if (cur_ret_type == TYPE_CHAR) {      // trunca para char (com sinal)
                    emit("slli a0, a0, 24");
                    emit("srai a0, a0, 24");
                }
            }
            // O epilogo vem logo depois do ultimo comando da funcao.
            if (!(q->next && q->next->op == IR_FUNC_END))
                emit("j .Lret_%s", cur_name);
            break;

        case IR_COMMENT:
            emit("# %s", q->text);
            break;

        default:
            cg_error("instrucao inesperada dentro de funcao", cur_name);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Funcoes                                                             */
/* ------------------------------------------------------------------ */

static void note_temp(const Operand *o) {
    if (o->kind == OPD_TEMP && o->value > temps_max) temps_max = o->value;
}

// Percorre a funcao uma vez para reservar o quadro: parametros, variaveis,
// vetores, temporarios e a area de argumentos de saida.
static void layout_function(const IRQuad *begin) {
    int used = 8;                    // ra e s0 do chamador
    int nformal = 0, max_args = 0;

    vt_clear(&locals);
    temps_max = 0;

    for (const IRQuad *q = begin->next; q && q->op != IR_FUNC_END; q = q->next) {
        note_temp(&q->arg1);
        note_temp(&q->arg2);
        note_temp(&q->res);
        switch (q->op) {
            case IR_FORMAL:
                if (nformal < MAX_REG_ARGS) {
                    used += 4;       // copiado de a<N> para o quadro na entrada
                    vt_add(&locals, q->res.name, -used, 0, 0, q->type);
                } else {             // ja esta na pilha do chamador
                    vt_add(&locals, q->res.name, (nformal - MAX_REG_ARGS) * 4, 0, 0, q->type);
                }
                nformal++;
                break;
            case IR_DECL:
                used += 4;
                vt_add(&locals, q->res.name, -used, 0, 0, q->type);
                break;
            case IR_DECL_ARRAY:
                // o vetor ocupa [s0 - used, s0 - used + bytes); o elemento 0 fica no menor endereco
                used += round_up(q->arg2.value * elem_size(q->type), 4);
                vt_add(&locals, q->res.name, -used, 1, q->arg2.value, q->type);
                break;
            case IR_CALL:
                if (q->arg2.value > max_args) max_args = q->arg2.value;
                break;
            default:
                break;
        }
    }

    temps_start = used;
    used += 4 * temps_max;
    if (max_args > MAX_REG_ARGS) used += 4 * (max_args - MAX_REG_ARGS);
    frame_size = round_up(used, 16);   // sp sempre alinhado em 16 bytes
}

static void gen_prologue(void) {
    if (frame_size <= 2048) {
        emit("addi sp, sp, -%d", frame_size);
        emit("sw ra, %d(sp)", frame_size - 4);
        emit("sw s0, %d(sp)", frame_size - 8);
        emit("addi s0, sp, %d", frame_size);
    } else {                          // quadro grande: deslocamentos nao cabem em 12 bits
        emit("mv t0, sp");
        emit("li t1, %d", frame_size);
        emit("sub sp, sp, t1");
        emit("sw ra, -4(t0)");
        emit("sw s0, -8(t0)");
        emit("mv s0, t0");
    }
}

static void gen_epilogue(void) {
    fprintf(out, ".Lret_%s:\n", cur_name);
    emit("lw ra, -4(s0)");
    emit("mv sp, s0");
    emit("lw s0, -8(sp)");
    emit("ret");
}

// Gera uma funcao inteira; devolve a quadrupla IR_FUNC_END (ou a ultima lida).
static const IRQuad *gen_function(const IRQuad *begin) {
    cur_name = begin->res.name;
    cur_ret_type = begin->type;
    param_index = 0;
    layout_function(begin);

    fprintf(out, "\n    .globl %s\n%s:\n", cur_name, cur_name);
    gen_prologue();

    // Parametros recebidos em a0-a7 vao para as posicoes reservadas no quadro.
    const IRQuad *q = begin->next;
    for (int i = 0; q && q->op == IR_FORMAL; q = q->next, i++) {
        if (i < MAX_REG_ARGS) {
            char reg[16];
            snprintf(reg, sizeof reg, "a%d", i);
            store_var(reg, q->res.name);
        }
    }

    const IRQuad *last = begin;
    for (; q && q->op != IR_FUNC_END; q = q->next) {
        gen_quad(q);
        last = q;
    }
    gen_epilogue();
    return q ? q : last;
}

/* ------------------------------------------------------------------ */
/* Dados globais e programa                                            */
/* ------------------------------------------------------------------ */

static void collect_globals(const IRProgram *prog) {
    for (const IRQuad *q = prog->head; q; q = q->next) {
        if (q->op == IR_GLOBAL)
            vt_add(&globals, q->res.name, 0, 0, 0, q->type);
        else if (q->op == IR_GLOBAL_ARRAY)
            vt_add(&globals, q->res.name, 0, 1, q->arg2.value, q->type);
    }
}

// Os IR_GLOBAL_INIT vem logo depois do IR_GLOBAL_ARRAY; o restante e zerado.
static void gen_global_array(const IRQuad *decl) {
    int size = decl->arg2.value;
    int esize = elem_size(decl->type);
    const char *directive = (esize == 1) ? ".byte" : ".word";

    int ninit = 0;
    for (const IRQuad *r = decl->next;
         r && r->op == IR_GLOBAL_INIT && strcmp(r->res.name, decl->res.name) == 0;
         r = r->next)
        if (r->arg2.value + 1 > ninit) ninit = r->arg2.value + 1;
    if (ninit > size) ninit = size;

    int *vals = (int *) calloc(ninit ? (size_t) ninit : 1, sizeof(int));
    if (!vals) { fprintf(stderr, "[ERRO CODIGO FINAL] memoria insuficiente.\n"); exit(1); }
    for (const IRQuad *r = decl->next;
         r && r->op == IR_GLOBAL_INIT && strcmp(r->res.name, decl->res.name) == 0;
         r = r->next)
        if (r->arg2.value >= 0 && r->arg2.value < ninit)
            vals[r->arg2.value] = (esize == 1) ? (int) (signed char) r->arg1.value : r->arg1.value;

    if (esize == 4) emit(".align 2");
    fprintf(out, "%s:\n", decl->res.name);
    for (int i = 0; i < ninit; i++) {
        if (i % 8 == 0) fprintf(out, "%s    %s ", i ? "\n" : "", directive);
        else            fprintf(out, ", ");
        fprintf(out, "%d", vals[i]);
    }
    if (ninit > 0) fputc('\n', out);
    if (size > ninit) emit(".space %d", (size - ninit) * esize);
    free(vals);
}

static void gen_data(const IRProgram *prog) {
    int started = 0;
    for (const IRQuad *q = prog->head; q; q = q->next) {
        if (q->op != IR_GLOBAL && q->op != IR_GLOBAL_ARRAY) continue;
        if (!started) { fprintf(out, "\n    .data\n"); started = 1; }

        if (q->op == IR_GLOBAL_ARRAY) {
            gen_global_array(q);
        } else if (q->type == TYPE_CHAR) {
            fprintf(out, "%s:\n", q->res.name);
            emit(".byte 0");
        } else {
            emit(".align 2");
            fprintf(out, "%s:\n", q->res.name);
            emit(".word 0");
        }
    }
}

static int has_function(const IRProgram *prog, const char *name) {
    for (const IRQuad *q = prog->head; q; q = q->next)
        if (q->op == IR_FUNC_BEGIN && strcmp(q->res.name, name) == 0) return 1;
    return 0;
}

static void gen_text(const IRProgram *prog) {
    fprintf(out, "\n    .text\n");

    // Ponto de entrada: chama main e termina com o valor retornado como codigo de saida.
    if (has_function(prog, "main")) {
        fprintf(out, "    .globl _start\n_start:\n");
        emit("jal ra, main");
        emit("li a7, 93");           // chamada de sistema exit (Linux); a0 = codigo de saida
        emit("ecall");
    }

    for (const IRQuad *q = prog->head; q; q = q->next) {
        if (q->op == IR_FUNC_BEGIN)
            q = gen_function(q);
        else if (q->op == IR_COMMENT)
            emit("# %s", q->text);
    }
}

AsmProgram *codegen_generate(const IRProgram *ir) {
    AsmProgram *code = (AsmProgram *) calloc(1, sizeof(AsmProgram));
    if (!code) { fprintf(stderr, "[ERRO CODIGO FINAL] memoria insuficiente.\n"); exit(1); }

    /* Use only standard C file operations; open_memstream is a GNU extension
       and is unavailable in some Windows toolchains (including MinGW). */
    out = tmpfile();
    if (!out) {
        fprintf(stderr, "[ERRO CODIGO FINAL] nao foi possivel criar arquivo temporario.\n");
        code->text = (char *) calloc(1, 1);
        code->errors = 1;
        return code;
    }
    errors = 0;
    param_index = 0;

    fprintf(out, "# Assembly RISC-V (RV32IM, ABI ilp32) gerado a partir do codigo intermediario\n");
    fprintf(out, "# Montar:  riscv64-unknown-elf-gcc -march=rv32im -mabi=ilp32 -nostdlib -static prog.s -o prog\n");
    fprintf(out, "# Executar: qemu-riscv32 ./prog   (codigo de saida = valor retornado por main)\n");

    if (ir) {
        collect_globals(ir);
        gen_data(ir);
        gen_text(ir);
    }

    if (fflush(out) != 0 || fseek(out, 0, SEEK_END) != 0) {
        fprintf(stderr, "[ERRO CODIGO FINAL] falha ao preparar a saida Assembly.\n");
        errors++;
    } else {
        long file_length = ftell(out);
        if (file_length < 0 || (unsigned long) file_length >= (unsigned long) ((size_t) -1)) {
            fprintf(stderr, "[ERRO CODIGO FINAL] tamanho invalido da saida Assembly.\n");
            errors++;
        } else {
            size_t length = (size_t) file_length;
            char *text = (char *) malloc(length + 1);
            if (!text) {
                fprintf(stderr, "[ERRO CODIGO FINAL] memoria insuficiente.\n");
                errors++;
            } else {
                rewind(out);
                size_t read_count = fread(text, 1, length, out);
                if (read_count != length || ferror(out)) {
                    fprintf(stderr, "[ERRO CODIGO FINAL] falha ao ler a saida Assembly.\n");
                    free(text);
                    errors++;
                } else {
                    text[length] = '\0';
                    code->text = text;
                    code->length = length;
                }
            }
        }
    }
    if (fclose(out) != 0) {
        fprintf(stderr, "[ERRO CODIGO FINAL] falha ao fechar a saida Assembly.\n");
        errors++;
    }
    out = NULL;
    code->errors = errors;
    if (!code->text) code->text = (char *) calloc(1, 1);

    vt_free(&globals);
    vt_free(&locals);
    return code;
}

/* ------------------------------------------------------------------ */
/* Impressao e liberacao                                               */
/* ------------------------------------------------------------------ */

void codegen_print(const AsmProgram *code, FILE *dst) {
    if (!code || !code->text || !dst) return;
    fputs(code->text, dst);
}

void codegen_free(AsmProgram *code) {
    if (!code) return;
    free(code->text);
    free(code);
}
