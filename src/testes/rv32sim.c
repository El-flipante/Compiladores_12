/*
 * rv32sim - simulador minimo de RISC-V 32 bits (RV32IM) para os testes.
 *
 * Le o Assembly (formato GNU as) que o compilador gera e o executa a partir
 * de _start, sem precisar de qemu nem de toolchain RISC-V. Quando o programa
 * chama exit (ecall com a7 = 93), imprime o valor de a0 como inteiro de 32
 * bits com sinal e termina com status 0.
 *
 * Cobre o subconjunto de instrucoes/pseudo-instrucoes que o gerador usa, e
 * falha (status != 0) quando o programa faz algo que no hardware real seria
 * erro ou comportamento indefinido:
 *   status 2  instrucao ou diretiva nao suportada / Assembly mal formado
 *   status 3  limite de passos excedido (provavel laco infinito)
 *   status 4  acesso a memoria fora dos limites ou desalinhado
 *   status 5  sp desalinhado (ABI exige 16 bytes) em uma chamada (jal)
 *   status 6  a execucao saiu do programa sem chamar exit (pc invalido)
 *
 * Uso: rv32sim [-v] prog.s     (-v: imprime tambem o numero de passos)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MEM_SIZE   (4u * 1024u * 1024u)
#define DATA_BASE  0x00010000u
#define MAX_STEPS  20000000L
#define MAX_INSNS  20000
#define MAX_LABELS 4000
#define MAX_TOKENS 64      // mnemonico + operandos (uma linha .word tem ate 9)

typedef enum {
    I_ADD, I_SUB, I_MUL, I_DIV, I_REM, I_SLT, I_SLTU, I_XOR, I_AND, I_OR,
    I_ADDI, I_XORI, I_ANDI, I_ORI, I_SLLI, I_SRLI, I_SRAI, I_SLTI,
    I_LW, I_LB, I_LBU, I_SW, I_SB,
    I_BEQ, I_BNE, I_BLT, I_BGE,
    I_JAL, I_JALR, I_LI, I_LA, I_ECALL
} Op;

typedef struct {
    Op op;
    int rd, rs1, rs2;
    int32_t imm;
    int target;            // indice da instrucao alvo (desvios e jal)
    char label[64];        // rotulo pendente (la, desvios, jal)
    int line;
} Insn;

typedef struct { char name[64]; int is_text; uint32_t value; } Label;

static uint8_t mem[MEM_SIZE];
static Insn insns[MAX_INSNS];
static int ninsns;
static Label labels[MAX_LABELS];
static int nlabels;
static uint32_t data_ptr = DATA_BASE;
static int in_text;

static const char *REGS[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2","s0","s1","a0","a1","a2","a3",
    "a4","a5","a6","a7","s2","s3","s4","s5","s6","s7","s8","s9","s10","s11",
    "t3","t4","t5","t6"
};

static void die(int status, int line, const char *msg, const char *detail) {
    fprintf(stderr, "[rv32sim] linha %d: %s%s%s\n", line, msg,
            detail ? ": " : "", detail ? detail : "");
    exit(status);
}

static int reg_num(const char *s, int line) {
    for (int i = 0; i < 32; i++)
        if (strcmp(s, REGS[i]) == 0) return i;
    if (strcmp(s, "fp") == 0) return 8;
    if (s[0] == 'x' && isdigit((unsigned char) s[1])) return atoi(s + 1);
    die(2, line, "registrador desconhecido", s);
    return 0;
}

static int find_label(const char *name) {
    for (int i = 0; i < nlabels; i++)
        if (strcmp(labels[i].name, name) == 0) return i;
    return -1;
}

static void add_label(const char *name, int line) {
    if (find_label(name) >= 0) die(2, line, "rotulo duplicado", name);
    if (nlabels == MAX_LABELS) die(2, line, "rotulos demais", NULL);
    snprintf(labels[nlabels].name, sizeof labels[nlabels].name, "%.63s", name);
    labels[nlabels].is_text = in_text;
    labels[nlabels].value = in_text ? (uint32_t) ninsns : data_ptr;
    nlabels++;
}

static void put_byte(uint8_t b, int line) {
    if (data_ptr >= MEM_SIZE / 2) die(2, line, "area de dados cheia", NULL);
    mem[data_ptr++] = b;
}

static void align_data(int bits) {
    uint32_t a = 1u << bits;
    while (data_ptr % a) put_byte(0, 0);
}

// Separa a linha em tokens: mnemonico e operandos (virgulas e parenteses viram separadores).
static int tokenize(char *line, char *tok[], int max) {
    int n = 0;
    char *p = line;
    while (*p && n < max) {
        while (*p == ' ' || *p == '\t' || *p == ',' || *p == '(' || *p == ')') p++;
        if (!*p) break;
        tok[n++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != ',' && *p != '(' && *p != ')') p++;
        if (*p) *p++ = '\0';
    }
    return n;
}

// Instrucoes tipo I, loads e stores so aceitam imediato de 12 bits com sinal
// (li e la sao pseudo-instrucoes e podem ter qualquer valor).
static void check_imm12(const Insn *in, const char *mnemonic) {
    if (in->imm < -2048 || in->imm > 2047)
        die(2, in->line, "imediato fora de 12 bits em", mnemonic);
}

static Insn *new_insn(Op op, int line) {
    if (ninsns == MAX_INSNS) die(2, line, "programa grande demais", NULL);
    Insn *in = &insns[ninsns++];
    memset(in, 0, sizeof *in);
    in->op = op;
    in->line = line;
    in->target = -1;
    return in;
}

static void set_label_ref(Insn *in, const char *name) {
    snprintf(in->label, sizeof in->label, "%.63s", name);
}

static void assemble_line(char *tok[], int n, int line) {
    const char *m = tok[0];
    Insn *in;

#define NEED(k) do { if (n != (k)) die(2, line, "numero de operandos invalido para", m); } while (0)
#define R3(NAME, OP) if (!strcmp(m, NAME)) { NEED(4); in = new_insn(OP, line); \
        in->rd = reg_num(tok[1], line); in->rs1 = reg_num(tok[2], line); \
        in->rs2 = reg_num(tok[3], line); return; }
#define RI(NAME, OP) if (!strcmp(m, NAME)) { NEED(4); in = new_insn(OP, line); \
        in->rd = reg_num(tok[1], line); in->rs1 = reg_num(tok[2], line); \
        in->imm = (int32_t) strtol(tok[3], NULL, 0); check_imm12(in, m); return; }
#define LD(NAME, OP) if (!strcmp(m, NAME)) { NEED(4); in = new_insn(OP, line); \
        in->rd = reg_num(tok[1], line); in->imm = (int32_t) strtol(tok[2], NULL, 0); \
        in->rs1 = reg_num(tok[3], line); check_imm12(in, m); return; }
#define ST(NAME, OP) if (!strcmp(m, NAME)) { NEED(4); in = new_insn(OP, line); \
        in->rs2 = reg_num(tok[1], line); in->imm = (int32_t) strtol(tok[2], NULL, 0); \
        in->rs1 = reg_num(tok[3], line); check_imm12(in, m); return; }
#define BR(NAME, OP) if (!strcmp(m, NAME)) { NEED(4); in = new_insn(OP, line); \
        in->rs1 = reg_num(tok[1], line); in->rs2 = reg_num(tok[2], line); \
        set_label_ref(in, tok[3]); return; }

    R3("add", I_ADD) R3("sub", I_SUB) R3("mul", I_MUL) R3("div", I_DIV)
    R3("rem", I_REM) R3("slt", I_SLT) R3("sltu", I_SLTU) R3("xor", I_XOR)
    R3("and", I_AND) R3("or", I_OR)
    RI("addi", I_ADDI) RI("xori", I_XORI) RI("andi", I_ANDI) RI("ori", I_ORI)
    RI("slli", I_SLLI) RI("srli", I_SRLI) RI("srai", I_SRAI) RI("slti", I_SLTI)
    LD("lw", I_LW) LD("lb", I_LB) LD("lbu", I_LBU)
    ST("sw", I_SW) ST("sb", I_SB)
    BR("beq", I_BEQ) BR("bne", I_BNE) BR("blt", I_BLT) BR("bge", I_BGE)

    if (!strcmp(m, "li")) {
        NEED(3); in = new_insn(I_LI, line);
        in->rd = reg_num(tok[1], line); in->imm = (int32_t) strtol(tok[2], NULL, 0); return;
    }
    if (!strcmp(m, "la")) {
        NEED(3); in = new_insn(I_LA, line);
        in->rd = reg_num(tok[1], line); set_label_ref(in, tok[2]); return;
    }
    if (!strcmp(m, "mv")) {
        NEED(3); in = new_insn(I_ADDI, line);
        in->rd = reg_num(tok[1], line); in->rs1 = reg_num(tok[2], line); return;
    }
    if (!strcmp(m, "neg")) {
        NEED(3); in = new_insn(I_SUB, line);
        in->rd = reg_num(tok[1], line); in->rs1 = 0; in->rs2 = reg_num(tok[2], line); return;
    }
    if (!strcmp(m, "seqz")) {      // sltiu rd, rs, 1
        NEED(3); in = new_insn(I_SLTU, line);
        in->rd = reg_num(tok[1], line); in->rs1 = reg_num(tok[2], line);
        in->rs2 = -1; in->imm = 1; return;
    }
    if (!strcmp(m, "snez")) {      // sltu rd, zero, rs
        NEED(3); in = new_insn(I_SLTU, line);
        in->rd = reg_num(tok[1], line); in->rs1 = 0; in->rs2 = reg_num(tok[2], line); return;
    }
    if (!strcmp(m, "beqz") || !strcmp(m, "bnez")) {
        NEED(3); in = new_insn(m[1] == 'e' ? I_BEQ : I_BNE, line);
        in->rs1 = reg_num(tok[1], line); in->rs2 = 0; set_label_ref(in, tok[2]); return;
    }
    if (!strcmp(m, "j")) {
        NEED(2); in = new_insn(I_JAL, line); in->rd = 0; set_label_ref(in, tok[1]); return;
    }
    if (!strcmp(m, "jal")) {
        if (n == 2) { in = new_insn(I_JAL, line); in->rd = 1; set_label_ref(in, tok[1]); return; }
        NEED(3); in = new_insn(I_JAL, line);
        in->rd = reg_num(tok[1], line); set_label_ref(in, tok[2]); return;
    }
    if (!strcmp(m, "ret")) { NEED(1); in = new_insn(I_JALR, line); in->rd = 0; in->rs1 = 1; return; }
    if (!strcmp(m, "ecall")) { NEED(1); new_insn(I_ECALL, line); return; }
    if (!strcmp(m, "nop")) { NEED(1); in = new_insn(I_ADDI, line); return; }
    die(2, line, "instrucao nao suportada", m);
}

static void directive(char *tok[], int n, int line) {
    const char *d = tok[0];
    if (!strcmp(d, ".data")) { in_text = 0; return; }
    if (!strcmp(d, ".text")) { in_text = 1; return; }
    if (!strcmp(d, ".globl") || !strcmp(d, ".global")) return;
    if (in_text) die(2, line, "diretiva de dados dentro de .text", d);
    if (!strcmp(d, ".align")) { if (n != 2) die(2, line, "uso de .align", NULL); align_data(atoi(tok[1])); return; }
    if (!strcmp(d, ".word")) {
        for (int i = 1; i < n; i++) {
            uint32_t v = (uint32_t) strtol(tok[i], NULL, 0);
            if (data_ptr % 4) die(2, line, ".word desalinhado (falta .align 2)", NULL);
            for (int b = 0; b < 4; b++) put_byte((uint8_t) (v >> (8 * b)), line);
        }
        return;
    }
    if (!strcmp(d, ".byte")) {
        for (int i = 1; i < n; i++) put_byte((uint8_t) strtol(tok[i], NULL, 0), line);
        return;
    }
    if (!strcmp(d, ".space") || !strcmp(d, ".zero")) {
        if (n != 2) die(2, line, "uso de .space", NULL);
        for (int i = atoi(tok[1]); i > 0; i--) put_byte(0, line);
        return;
    }
    die(2, line, "diretiva nao suportada", d);
}

static void load_program(FILE *f) {
    char buf[512];
    int line = 0;
    in_text = 1;
    while (fgets(buf, sizeof buf, f)) {
        line++;
        char *hash = strchr(buf, '#');
        if (hash) *hash = '\0';
        buf[strcspn(buf, "\r\n")] = '\0';
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        // rotulos no inicio da linha (pode haver mais de um e instrucao depois)
        for (;;) {
            char *colon = strchr(p, ':');
            char *q = p;
            while (q < colon && (isalnum((unsigned char) *q) || *q == '_' || *q == '.' || *q == '$')) q++;
            if (!colon || q != colon) break;
            *colon = '\0';
            add_label(p, line);
            p = colon + 1;
            while (*p == ' ' || *p == '\t') p++;
        }
        char *tok[MAX_TOKENS];
        int n = tokenize(p, tok, MAX_TOKENS);
        if (n == 0) continue;
        if (tok[0][0] == '.') directive(tok, n, line);
        else if (!in_text) die(2, line, "instrucao fora de .text", tok[0]);
        else assemble_line(tok, n, line);
    }
    for (int i = 0; i < ninsns; i++) {
        if (!insns[i].label[0]) continue;
        int l = find_label(insns[i].label);
        if (l < 0) die(2, insns[i].line, "rotulo indefinido", insns[i].label);
        if (insns[i].op == I_LA) insns[i].imm = (int32_t) labels[l].value;
        else if (!labels[l].is_text) die(2, insns[i].line, "desvio para rotulo de dados", insns[i].label);
        else insns[i].target = (int) labels[l].value;
    }
}

static uint32_t reg[32];
static int cur_line;

static void check_mem(uint32_t addr, int size, const char *what) {
    if ((uint64_t) addr + size > MEM_SIZE || addr < 0x1000u) {
        char d[64]; snprintf(d, sizeof d, "%s em 0x%08x", what, addr);
        die(4, cur_line, "acesso a memoria invalido", d);
    }
    if (addr % size) {
        char d[64]; snprintf(d, sizeof d, "%s em 0x%08x", what, addr);
        die(4, cur_line, "acesso desalinhado", d);
    }
}

static int32_t do_div(int32_t a, int32_t b) {
    if (b == 0) return -1;
    if (a == INT32_MIN && b == -1) return INT32_MIN;
    return a / b;
}

static int32_t do_rem(int32_t a, int32_t b) {
    if (b == 0) return a;
    if (a == INT32_MIN && b == -1) return 0;
    return a % b;
}

int main(int argc, char **argv) {
    int verbose = 0;
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) verbose = 1; else path = argv[i];
    }
    if (!path) { fprintf(stderr, "uso: rv32sim [-v] prog.s\n"); return 2; }
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "[rv32sim] nao foi possivel abrir %s\n", path); return 2; }
    load_program(f);
    fclose(f);

    int start = find_label("_start");
    if (start < 0 || !labels[start].is_text) die(2, 0, "rotulo _start ausente", NULL);

    int pc = (int) labels[start].value;
    reg[2] = MEM_SIZE;                      // sp no topo da memoria (alinhado em 16)
    long steps = 0;

    for (;;) {
        if (pc < 0 || pc >= ninsns) die(6, cur_line, "execucao saiu do programa sem chamar exit", NULL);
        if (++steps > MAX_STEPS) die(3, insns[pc].line, "limite de passos excedido (laco infinito?)", NULL);
        const Insn *in = &insns[pc];
        cur_line = in->line;
        int next = pc + 1;
        uint32_t a = reg[in->rs1], b = reg[in->rs2];
        uint32_t addr;
        switch (in->op) {
            case I_ADD:  reg[in->rd] = a + b; break;
            case I_SUB:  reg[in->rd] = a - b; break;
            case I_MUL:  reg[in->rd] = (uint32_t) ((int32_t) a * (int64_t) (int32_t) b); break;
            case I_DIV:  reg[in->rd] = (uint32_t) do_div((int32_t) a, (int32_t) b); break;
            case I_REM:  reg[in->rd] = (uint32_t) do_rem((int32_t) a, (int32_t) b); break;
            case I_SLT:  reg[in->rd] = (int32_t) a < (int32_t) b; break;
            case I_SLTU: reg[in->rd] = a < (in->rs2 < 0 ? (uint32_t) in->imm : b); break;
            case I_XOR:  reg[in->rd] = a ^ b; break;
            case I_AND:  reg[in->rd] = a & b; break;
            case I_OR:   reg[in->rd] = a | b; break;
            case I_ADDI: reg[in->rd] = a + (uint32_t) in->imm; break;
            case I_XORI: reg[in->rd] = a ^ (uint32_t) in->imm; break;
            case I_ANDI: reg[in->rd] = a & (uint32_t) in->imm; break;
            case I_ORI:  reg[in->rd] = a | (uint32_t) in->imm; break;
            case I_SLLI: reg[in->rd] = a << (in->imm & 31); break;
            case I_SRLI: reg[in->rd] = a >> (in->imm & 31); break;
            case I_SRAI: reg[in->rd] = (uint32_t) ((int32_t) a >> (in->imm & 31)); break;
            case I_SLTI: reg[in->rd] = (int32_t) a < in->imm; break;
            case I_LI:   reg[in->rd] = (uint32_t) in->imm; break;
            case I_LA:   reg[in->rd] = (uint32_t) in->imm; break;
            case I_LW:
                addr = a + (uint32_t) in->imm; check_mem(addr, 4, "lw");
                reg[in->rd] = (uint32_t) mem[addr] | (uint32_t) mem[addr + 1] << 8 |
                              (uint32_t) mem[addr + 2] << 16 | (uint32_t) mem[addr + 3] << 24;
                break;
            case I_LB:
                addr = a + (uint32_t) in->imm; check_mem(addr, 1, "lb");
                reg[in->rd] = (uint32_t) (int32_t) (int8_t) mem[addr];
                break;
            case I_LBU:
                addr = a + (uint32_t) in->imm; check_mem(addr, 1, "lbu");
                reg[in->rd] = mem[addr];
                break;
            case I_SW:
                addr = a + (uint32_t) in->imm; check_mem(addr, 4, "sw");
                for (int k = 0; k < 4; k++) mem[addr + k] = (uint8_t) (b >> (8 * k));
                break;
            case I_SB:
                addr = a + (uint32_t) in->imm; check_mem(addr, 1, "sb");
                mem[addr] = (uint8_t) b;
                break;
            case I_BEQ: if (a == b) next = in->target; break;
            case I_BNE: if (a != b) next = in->target; break;
            case I_BLT: if ((int32_t) a < (int32_t) b) next = in->target; break;
            case I_BGE: if ((int32_t) a >= (int32_t) b) next = in->target; break;
            case I_JAL:
                if (in->rd == 1 && reg[2] % 16 != 0)
                    die(5, in->line, "sp desalinhado em uma chamada (ABI exige multiplo de 16)", NULL);
                reg[in->rd] = (uint32_t) (pc + 1);
                next = in->target;
                break;
            case I_JALR: {
                // so e usado como 'ret': o endereco de retorno e um indice de instrucao
                int dest = (int) (a + (uint32_t) in->imm);
                reg[in->rd] = (uint32_t) (pc + 1);
                next = dest;
                break;
            }
            case I_ECALL:
                if (reg[17] == 93) {
                    printf("%d\n", (int32_t) reg[10]);
                    if (verbose) fprintf(stderr, "[rv32sim] %ld passos\n", steps);
                    return 0;
                }
                die(2, in->line, "ecall nao suportada (a7 != 93)", NULL);
                break;
        }
        reg[0] = 0;
        pc = next;
    }
}
