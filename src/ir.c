#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ast.h"
#include "ir.h"
#include "parser.tab.h"   // codigos dos operadores (PLUS, MINUS, LT, ...)

/* ------------------------------------------------------------------ */
/* Estado da geracao                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    char *src;        // nome no codigo-fonte
    char *ir;         // nome unico usado no codigo intermediario
    int is_array;
    int size;
    DataType type;
} VarInfo;

typedef struct {
    char *name;
    int count;        // quantas vezes o nome ja foi usado
} NameCount;

typedef struct {
    NameCount *v;
    int n, cap;
} NameTable;

static IRProgram *prog;

static VarInfo *vars;          // pilha de escopos (globais na base)
static int nvars, capvars;

static NameTable global_names; // nomes de todas as variaveis globais
static NameTable func_names;   // nomes ja usados na funcao corrente

static int temp_counter;       // reinicia a cada funcao
static int label_counter;      // unico no programa

static void gen_stmt(ASTNode *n);
static Operand gen_expr(ASTNode *n);

/* ------------------------------------------------------------------ */
/* Utilitarios                                                         */
/* ------------------------------------------------------------------ */

static char *dup_str(const char *s) {
    char *c = strdup(s);
    if (!c) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }
    return c;
}

static void gen_error(const char *msg, const char *detail) {
    if (detail) fprintf(stderr, "[ERRO GERACAO] %s '%s'.\n", msg, detail);
    else        fprintf(stderr, "[ERRO GERACAO] %s.\n", msg);
    prog->errors++;
}

static Operand opd_none(void)        { Operand o = { OPD_NONE, 0, NULL }; return o; }
static Operand opd_const(int v)      { Operand o = { OPD_CONST, v, NULL }; return o; }
static Operand opd_temp(int n)       { Operand o = { OPD_TEMP, n, NULL }; return o; }
static Operand opd_label(int n)      { Operand o = { OPD_LABEL, n, NULL }; return o; }
static Operand opd_var(const char *s){ Operand o = { OPD_VAR, 0, dup_str(s) }; return o; }
static Operand opd_func(const char *s){ Operand o = { OPD_FUNC, 0, dup_str(s) }; return o; }

static Operand new_temp(void)  { return opd_temp(++temp_counter); }
static Operand new_label(void) { return opd_label(++label_counter); }

static IRQuad *emit(IROp op, Operand a1, Operand a2, Operand res, DataType type) {
    IRQuad *q = (IRQuad *) calloc(1, sizeof(IRQuad));
    if (!q) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }
    q->op = op;
    q->arg1 = a1;
    q->arg2 = a2;
    q->res = res;
    q->type = type;
    if (prog->tail) prog->tail->next = q; else prog->head = q;
    prog->tail = q;
    prog->count++;
    return q;
}

static void emit_comment(const char *text) {
    IRQuad *q = emit(IR_COMMENT, opd_none(), opd_none(), opd_none(), TYPE_UNKNOWN);
    q->text = dup_str(text);
}

/* ------------------------------------------------------------------ */
/* Nomes unicos e escopos                                              */
/* ------------------------------------------------------------------ */

static int nt_count(const NameTable *t, const char *name) {
    for (int i = 0; i < t->n; i++)
        if (strcmp(t->v[i].name, name) == 0) return t->v[i].count;
    return 0;
}

static void nt_bump(NameTable *t, const char *name) {
    for (int i = 0; i < t->n; i++) {
        if (strcmp(t->v[i].name, name) == 0) { t->v[i].count++; return; }
    }
    if (t->n == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 16;
        t->v = (NameCount *) realloc(t->v, (size_t) t->cap * sizeof(NameCount));
        if (!t->v) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }
    }
    t->v[t->n].name = dup_str(name);
    t->v[t->n].count = 1;
    t->n++;
}

static void nt_clear(NameTable *t) {
    for (int i = 0; i < t->n; i++) free(t->v[i].name);
    t->n = 0;
}

/*
 * Variaveis de escopos diferentes podem ter o mesmo nome no fonte (sombra).
 * No codigo intermediario cada declaracao recebe um nome unico dentro da
 * funcao: o primeiro uso mantem o nome (x), os seguintes viram x.1, x.2, ...
 * ('.' nao ocorre em identificadores C, entao nao ha colisao).
 */
static char *unique_name(const char *src, int is_global) {
    if (is_global) return dup_str(src);   // nome ja reservado em reserve_global_names
    int used = nt_count(&global_names, src) + nt_count(&func_names, src);
    char *name;
    if (used == 0) {
        name = dup_str(src);
    } else {
        size_t len = strlen(src) + 16;
        name = (char *) malloc(len);
        if (!name) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }
        snprintf(name, len, "%s.%d", src, used);
    }
    nt_bump(&func_names, src);
    return name;
}

static VarInfo *declare_var(const char *src, int is_array, int size, DataType type, int is_global) {
    if (nvars == capvars) {
        capvars = capvars ? capvars * 2 : 32;
        vars = (VarInfo *) realloc(vars, (size_t) capvars * sizeof(VarInfo));
        if (!vars) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }
    }
    VarInfo *v = &vars[nvars++];
    v->src = dup_str(src);
    v->ir = unique_name(src, is_global);
    v->is_array = is_array;
    v->size = size;
    v->type = type;
    return v;
}

static VarInfo *find_var(const char *src) {
    for (int i = nvars - 1; i >= 0; i--)
        if (strcmp(vars[i].src, src) == 0) return &vars[i];
    return NULL;
}

static int scope_enter(void) { return nvars; }

static void scope_leave(int mark) {
    while (nvars > mark) {
        nvars--;
        free(vars[nvars].src);
        free(vars[nvars].ir);
    }
}

/* ------------------------------------------------------------------ */
/* Listas da AST (parametros e argumentos), na ordem do codigo-fonte   */
/* ------------------------------------------------------------------ */

typedef struct {
    ASTNode **v;
    int n, cap;
} NodeList;

static void nl_push(NodeList *l, ASTNode *n) {
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->v = (ASTNode **) realloc(l->v, (size_t) l->cap * sizeof(ASTNode *));
        if (!l->v) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }
    }
    l->v[l->n++] = n;
}

// As listas sao encadeadas "do ultimo para o primeiro" (next = anteriores).
static void collect_args(ASTNode *n, NodeList *l) {
    if (!n) return;
    if (n->type == AST_ARGS_LIST) {
        collect_args(n->args_list.next, l);
        nl_push(l, n->args_list.expr);
    } else {
        nl_push(l, n);
    }
}

static void collect_params(ASTNode *n, NodeList *l) {
    if (!n) return;
    if (n->type == AST_PARAM_LIST) {
        collect_params(n->param_list.next, l);
        nl_push(l, n->param_list.param);
    } else {
        nl_push(l, n);
    }
}

/* ------------------------------------------------------------------ */
/* Expressoes                                                          */
/* ------------------------------------------------------------------ */

static int binop_to_ir(int op, IROp *out) {
    switch (op) {
        case PLUS:   *out = IR_ADD; return 1;
        case MINUS:  *out = IR_SUB; return 1;
        case TIMES:  *out = IR_MUL; return 1;
        case DIVIDE: *out = IR_DIV; return 1;
        case MOD:    *out = IR_MOD; return 1;
        case LT:     *out = IR_LT;  return 1;
        case GT:     *out = IR_GT;  return 1;
        case LE:     *out = IR_LE;  return 1;
        case GE:     *out = IR_GE;  return 1;
        case EQ:     *out = IR_EQ;  return 1;
        case NE:     *out = IR_NE;  return 1;
        default: return 0;
    }
}

static Operand gen_expr(ASTNode *n) {
    if (!n) { gen_error("expressao ausente na AST", NULL); return opd_const(0); }

    switch (n->type) {
        case AST_NUM:
        case AST_CHAR:
            return opd_const(n->intval);

        case AST_ID: {
            VarInfo *v = find_var(n->id);
            if (!v) { gen_error("variavel nao declarada", n->id); return opd_const(0); }
            return opd_var(v->ir);
        }

        case AST_UNOP: {
            if (n->unop.op != MINUS) { gen_error("operador unario nao suportado", NULL); return opd_const(0); }
            Operand a = gen_expr(n->unop.expr);
            Operand t = new_temp();
            emit(IR_NEG, a, opd_none(), t, TYPE_UNKNOWN);
            return t;
        }

        case AST_BINOP: {
            IROp op;
            if (!binop_to_ir(n->binop.op, &op)) { gen_error("operador binario nao suportado", NULL); return opd_const(0); }
            Operand l = gen_expr(n->binop.left);
            Operand r = gen_expr(n->binop.right);
            Operand t = new_temp();
            emit(op, l, r, t, TYPE_UNKNOWN);
            return t;
        }

        case AST_ARRAY_ACCESS: {
            VarInfo *v = find_var(n->array_access.id);
            if (!v || !v->is_array) { gen_error("vetor nao declarado", n->array_access.id); return opd_const(0); }
            Operand idx = gen_expr(n->array_access.index_expr);
            Operand t = new_temp();
            emit(IR_ARRAY_LOAD, opd_var(v->ir), idx, t, v->type);
            return t;
        }

        case AST_FUNC_CALL: {
            // Os argumentos sao capturados antes dos 'param': assim chamadas
            // aninhadas nao misturam parametros, e efeitos colaterais posteriores
            // nao alteram valores de argumentos ja avaliados.
            NodeList args = {0};
            collect_args(n->func_call.args, &args);
            Operand *vals = (Operand *) calloc(args.n ? (size_t) args.n : 1, sizeof(Operand));
            if (!vals) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }
            for (int i = 0; i < args.n; i++) {
                Operand value = gen_expr(args.v[i]);
                // Capture the value now: later arguments may call functions that
                // modify a variable referenced by an earlier argument.
                vals[i] = new_temp();
                emit(IR_COPY, value, opd_none(), vals[i], TYPE_UNKNOWN);
            }
            for (int i = 0; i < args.n; i++)
                emit(IR_PARAM, vals[i], opd_none(), opd_none(), TYPE_UNKNOWN);
            Operand res = (n->expr_type == TYPE_VOID) ? opd_none() : new_temp();
            emit(IR_CALL, opd_func(n->func_call.id), opd_const(args.n), res, TYPE_UNKNOWN);
            free(vals);
            free(args.v);
            return res;
        }

        default:
            gen_error("expressao nao suportada pelo gerador de codigo intermediario", NULL);
            return opd_const(0);
    }
}

/* ------------------------------------------------------------------ */
/* Comandos                                                            */
/* ------------------------------------------------------------------ */

static void gen_local_decl(ASTNode *n) {
    VarInfo *v = declare_var(n->decl.id, n->decl.is_array, n->decl.array_size,
                             n->decl.data_type, 0);
    if (!v->is_array) {
        emit(IR_DECL, opd_none(), opd_none(), opd_var(v->ir), v->type);
        return;
    }
    emit(IR_DECL_ARRAY, opd_none(), opd_const(v->size), opd_var(v->ir), v->type);

    if (n->decl.init_list) {
        // Elementos listados recebem o valor; os demais sao zerados (semantica de C).
        NodeList items = {0};
        collect_args(n->decl.init_list, &items);
        for (int i = 0; i < items.n; i++) {
            Operand val = gen_expr(items.v[i]);
            emit(IR_ARRAY_STORE, val, opd_const(i), opd_var(v->ir), v->type);
        }
        for (int i = items.n; i < v->size; i++)
            emit(IR_ARRAY_STORE, opd_const(0), opd_const(i), opd_var(v->ir), v->type);
        free(items.v);
    }
}

static void gen_assign(ASTNode *n) {
    VarInfo *v = find_var(n->assign.id);
    if (!v) { gen_error("variavel nao declarada", n->assign.id); return; }

    if (n->assign.index_expr) {
        if (!v->is_array) { gen_error("identificador nao e um vetor", v->src); return; }
        Operand idx = gen_expr(n->assign.index_expr);
        Operand val = gen_expr(n->assign.expr);
        emit(IR_ARRAY_STORE, val, idx, opd_var(v->ir), v->type);
    } else {
        Operand val = gen_expr(n->assign.expr);
        emit(IR_COPY, val, opd_none(), opd_var(v->ir), v->type);
    }
}

/*
 * 'for (int i = expr; ...)' chega como SEQ(DECL i, ASSIGN i expr). O parser
 * resolve 'expr' ANTES de declarar 'i'; aqui fazemos o mesmo, avaliando 'expr'
 * antes de declarar a variavel (importa quando 'i' sombreia outra variavel).
 */
static void gen_for_init(ASTNode *init) {
    if (!init) return;
    if (init->type == AST_SEQ &&
        init->seq.stmt1 && init->seq.stmt1->type == AST_DECL &&
        init->seq.stmt2 && init->seq.stmt2->type == AST_ASSIGN &&
        !init->seq.stmt2->assign.index_expr &&
        strcmp(init->seq.stmt1->decl.id, init->seq.stmt2->assign.id) == 0) {
        Operand val = gen_expr(init->seq.stmt2->assign.expr);
        VarInfo *v = declare_var(init->seq.stmt1->decl.id, 0, 0,
                                 init->seq.stmt1->decl.data_type, 0);
        emit(IR_DECL, opd_none(), opd_none(), opd_var(v->ir), v->type);
        emit(IR_COPY, val, opd_none(), opd_var(v->ir), v->type);
        return;
    }
    gen_stmt(init);
}

static void gen_stmt(ASTNode *n) {
    if (!n) return;

    switch (n->type) {
        case AST_SEQ:
            gen_stmt(n->seq.stmt1);
            gen_stmt(n->seq.stmt2);
            break;

        case AST_BLOCK: {
            int mark = scope_enter();
            gen_stmt(n->block.body);
            scope_leave(mark);
            break;
        }

        case AST_DECL:
            gen_local_decl(n);
            break;

        case AST_ASSIGN:
            gen_assign(n);
            break;

        case AST_IF: {
            Operand cond = gen_expr(n->if_stmt.cond);
            Operand l_end = new_label();
            if (n->if_stmt.else_branch) {
                Operand l_else = new_label();
                emit(IR_IF_FALSE, cond, opd_none(), l_else, TYPE_UNKNOWN);
                gen_stmt(n->if_stmt.if_branch);
                emit(IR_GOTO, opd_none(), opd_none(), l_end, TYPE_UNKNOWN);
                emit(IR_LABEL, opd_none(), opd_none(), l_else, TYPE_UNKNOWN);
                gen_stmt(n->if_stmt.else_branch);
            } else {
                emit(IR_IF_FALSE, cond, opd_none(), l_end, TYPE_UNKNOWN);
                gen_stmt(n->if_stmt.if_branch);
            }
            emit(IR_LABEL, opd_none(), opd_none(), l_end, TYPE_UNKNOWN);
            break;
        }

        case AST_WHILE: {
            Operand l_begin = new_label();
            Operand l_end = new_label();
            emit(IR_LABEL, opd_none(), opd_none(), l_begin, TYPE_UNKNOWN);
            Operand cond = gen_expr(n->while_stmt.cond);
            emit(IR_IF_FALSE, cond, opd_none(), l_end, TYPE_UNKNOWN);
            gen_stmt(n->while_stmt.body);
            emit(IR_GOTO, opd_none(), opd_none(), l_begin, TYPE_UNKNOWN);
            emit(IR_LABEL, opd_none(), opd_none(), l_end, TYPE_UNKNOWN);
            break;
        }

        case AST_FOR: {
            int mark = scope_enter();          // a variavel do 'for' vive so no laco
            gen_for_init(n->for_stmt.init);
            Operand l_cond = new_label();
            Operand l_end = new_label();
            emit(IR_LABEL, opd_none(), opd_none(), l_cond, TYPE_UNKNOWN);
            if (n->for_stmt.cond) {            // condicao ausente = laco infinito
                Operand cond = gen_expr(n->for_stmt.cond);
                emit(IR_IF_FALSE, cond, opd_none(), l_end, TYPE_UNKNOWN);
            }
            gen_stmt(n->for_stmt.body);
            gen_stmt(n->for_stmt.inc);
            emit(IR_GOTO, opd_none(), opd_none(), l_cond, TYPE_UNKNOWN);
            emit(IR_LABEL, opd_none(), opd_none(), l_end, TYPE_UNKNOWN);
            scope_leave(mark);
            break;
        }

        case AST_RETURN:
            if (n->return_stmt.expr) {
                Operand v = gen_expr(n->return_stmt.expr);
                emit(IR_RETURN, v, opd_none(), opd_none(), TYPE_UNKNOWN);
            } else {
                emit(IR_RETURN, opd_none(), opd_none(), opd_none(), TYPE_UNKNOWN);
            }
            break;

        case AST_FUNC_DECL:
            // Prototipo nao gera codigo. Funcao aninhada nao faz parte da linguagem.
            if (n->func_decl.body)
                emit_comment("AVISO: definicao de funcao dentro de funcao nao faz parte da linguagem; ignorada");
            break;

        case AST_NUM: case AST_CHAR: case AST_ID: case AST_UNOP: case AST_BINOP:
        case AST_ARRAY_ACCESS: case AST_FUNC_CALL:
            {   // expressao como comando: avalia (efeitos colaterais) e descarta o resultado
                Operand discarded = gen_expr(n);
                free(discarded.name);
            }
            break;

        default:
            gen_error("comando nao suportado pelo gerador de codigo intermediario", NULL);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Funcoes e escopo global                                             */
/* ------------------------------------------------------------------ */

static void gen_function(ASTNode *n) {
    if (!n->func_decl.body) return;            // prototipo: nada a gerar

    int mark = scope_enter();
    nt_clear(&func_names);
    temp_counter = 0;

    NodeList params = {0};
    collect_params(n->func_decl.params, &params);

    emit(IR_FUNC_BEGIN, opd_none(), opd_const(params.n),
         opd_func(n->func_decl.id), n->func_decl.return_type);
    for (int i = 0; i < params.n; i++) {
        ASTNode *p = params.v[i];
        VarInfo *v = declare_var(p->param.id, 0, 0, p->param.type, 0);
        emit(IR_FORMAL, opd_none(), opd_none(), opd_var(v->ir), v->type);
    }
    free(params.v);

    gen_stmt(n->func_decl.body);

    // Funcao void pode terminar sem 'return': o retorno e explicito no codigo.
    if (n->func_decl.return_type == TYPE_VOID &&
        !(prog->tail && prog->tail->op == IR_RETURN))
        emit(IR_RETURN, opd_none(), opd_none(), opd_none(), TYPE_UNKNOWN);

    emit(IR_FUNC_END, opd_none(), opd_none(), opd_func(n->func_decl.id), TYPE_UNKNOWN);
    scope_leave(mark);
}

// Avalia expressoes constantes inteiras permitidas em inicializadores globais.
static int const_value(ASTNode *n, int *out) {
    if (!n) return 0;
    if (n->type == AST_NUM || n->type == AST_CHAR) { *out = n->intval; return 1; }
    if (n->type == AST_UNOP && n->unop.op == MINUS) {
        int v;
        if (!const_value(n->unop.expr, &v)) return 0;
        if (v == INT_MIN) return 0;
        *out = -v;
        return 1;
    }
    if (n->type == AST_BINOP) {
        int left, right;
        if (!const_value(n->binop.left, &left) ||
            !const_value(n->binop.right, &right)) return 0;

        long long value;
        switch (n->binop.op) {
            case PLUS:   value = (long long) left + right; break;
            case MINUS:  value = (long long) left - right; break;
            case TIMES:  value = (long long) left * right; break;
            case DIVIDE:
                if (right == 0 || (left == INT_MIN && right == -1)) return 0;
                value = left / right;
                break;
            case MOD:
                if (right == 0 || (left == INT_MIN && right == -1)) return 0;
                value = left % right;
                break;
            case LT: value = left < right; break;
            case GT: value = left > right; break;
            case LE: value = left <= right; break;
            case GE: value = left >= right; break;
            case EQ: value = left == right; break;
            case NE: value = left != right; break;
            default: return 0;
        }
        if (value < INT_MIN || value > INT_MAX) return 0;
        *out = (int) value;
        return 1;
    }
    return 0;
}

static void gen_global_decl(ASTNode *n) {
    VarInfo *v = declare_var(n->decl.id, n->decl.is_array, n->decl.array_size,
                             n->decl.data_type, 1);
    if (!v->is_array) {
        emit(IR_GLOBAL, opd_none(), opd_none(), opd_var(v->ir), v->type);
        return;
    }
    emit(IR_GLOBAL_ARRAY, opd_none(), opd_const(v->size), opd_var(v->ir), v->type);

    if (n->decl.init_list) {
        NodeList items = {0};
        collect_args(n->decl.init_list, &items);
        for (int i = 0; i < items.n; i++) {
            int val;
            if (!const_value(items.v[i], &val)) {
                gen_error("inicializador global nao constante ou fora do intervalo suportado",
                          n->decl.id);
                break;
            }
            emit(IR_GLOBAL_INIT, opd_const(val), opd_const(i), opd_var(v->ir), v->type);
        }
        free(items.v);
    }
}

// Passo 1: reserva os nomes de todas as globais (um local nunca colide com uma global
// declarada depois da funcao).
static void reserve_global_names(ASTNode *n) {
    if (!n) return;
    if (n->type == AST_SEQ) {
        reserve_global_names(n->seq.stmt1);
        reserve_global_names(n->seq.stmt2);
    } else if (n->type == AST_DECL) {
        nt_bump(&global_names, n->decl.id);
    }
}

// Passo 2: o programa e uma sequencia de declaracoes globais e funcoes.
static void gen_toplevel(ASTNode *n) {
    if (!n) return;
    switch (n->type) {
        case AST_SEQ:
            gen_toplevel(n->seq.stmt1);
            gen_toplevel(n->seq.stmt2);
            break;
        case AST_DECL:
            gen_global_decl(n);
            break;
        case AST_FUNC_DECL:
            gen_function(n);
            break;
        default:
            // O parser aceita comandos soltos, mas a linguagem so admite
            // declaracoes e funcoes no nivel do programa.
            emit_comment("AVISO: comando fora de funcao nao faz parte da linguagem; ignorado");
            break;
    }
}

IRProgram *ir_generate(ASTNode *root) {
    prog = (IRProgram *) calloc(1, sizeof(IRProgram));
    if (!prog) { fprintf(stderr, "[ERRO GERACAO] memoria insuficiente.\n"); exit(1); }

    nvars = 0;
    temp_counter = 0;
    label_counter = 0;
    nt_clear(&global_names);
    nt_clear(&func_names);

    reserve_global_names(root);
    nt_clear(&func_names);
    gen_toplevel(root);

    scope_leave(0);
    nt_clear(&global_names);
    nt_clear(&func_names);

    IRProgram *result = prog;
    prog = NULL;
    return result;
}

/* ------------------------------------------------------------------ */
/* Impressao e liberacao                                               */
/* ------------------------------------------------------------------ */

static void print_operand(FILE *out, const Operand *o) {
    switch (o->kind) {
        case OPD_TEMP:  fprintf(out, "t%d", o->value); break;
        case OPD_VAR:
        case OPD_FUNC:  fprintf(out, "%s", o->name); break;
        case OPD_CONST: fprintf(out, "%d", o->value); break;
        case OPD_LABEL: fprintf(out, "L%d", o->value); break;
        case OPD_NONE:  break;
    }
}

static const char *binop_symbol(IROp op) {
    switch (op) {
        case IR_ADD: return "+";
        case IR_SUB: return "-";
        case IR_MUL: return "*";
        case IR_DIV: return "/";
        case IR_MOD: return "%";
        case IR_LT:  return "<";
        case IR_GT:  return ">";
        case IR_LE:  return "<=";
        case IR_GE:  return ">=";
        case IR_EQ:  return "==";
        case IR_NE:  return "!=";
        default:     return "?";
    }
}

static void print_quad(FILE *out, const IRQuad *q) {
    switch (q->op) {
        case IR_FUNC_BEGIN:
            fprintf(out, "func %s : %s\n", q->res.name, datatype_to_str(q->type));
            break;
        case IR_FUNC_END:
            fprintf(out, "endfunc\n");
            break;
        case IR_FORMAL:
            fprintf(out, "    formal %s : %s\n", q->res.name, datatype_to_str(q->type));
            break;
        case IR_GLOBAL:
            fprintf(out, "global %s : %s\n", q->res.name, datatype_to_str(q->type));
            break;
        case IR_GLOBAL_ARRAY:
            fprintf(out, "global_array %s[%d] : %s\n", q->res.name, q->arg2.value, datatype_to_str(q->type));
            break;
        case IR_GLOBAL_INIT:
            fprintf(out, "global_init %s[%d] = %d\n", q->res.name, q->arg2.value, q->arg1.value);
            break;
        case IR_DECL:
            fprintf(out, "    decl %s : %s\n", q->res.name, datatype_to_str(q->type));
            break;
        case IR_DECL_ARRAY:
            fprintf(out, "    array %s[%d] : %s\n", q->res.name, q->arg2.value, datatype_to_str(q->type));
            break;
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_DIV: case IR_MOD:
        case IR_LT: case IR_GT: case IR_LE: case IR_GE: case IR_EQ: case IR_NE:
            fprintf(out, "    ");
            print_operand(out, &q->res);
            fprintf(out, " = ");
            print_operand(out, &q->arg1);
            fprintf(out, " %s ", binop_symbol(q->op));
            print_operand(out, &q->arg2);
            fprintf(out, "\n");
            break;
        case IR_NEG:
            fprintf(out, "    ");
            print_operand(out, &q->res);
            fprintf(out, " = -");
            print_operand(out, &q->arg1);
            fprintf(out, "\n");
            break;
        case IR_COPY:
            fprintf(out, "    ");
            print_operand(out, &q->res);
            fprintf(out, " = ");
            print_operand(out, &q->arg1);
            fprintf(out, "\n");
            break;
        case IR_ARRAY_LOAD:
            fprintf(out, "    ");
            print_operand(out, &q->res);
            fprintf(out, " = ");
            print_operand(out, &q->arg1);
            fprintf(out, "[");
            print_operand(out, &q->arg2);
            fprintf(out, "]\n");
            break;
        case IR_ARRAY_STORE:
            fprintf(out, "    ");
            print_operand(out, &q->res);
            fprintf(out, "[");
            print_operand(out, &q->arg2);
            fprintf(out, "] = ");
            print_operand(out, &q->arg1);
            fprintf(out, "\n");
            break;
        case IR_LABEL:
            fprintf(out, "  ");
            print_operand(out, &q->res);
            fprintf(out, ":\n");
            break;
        case IR_GOTO:
            fprintf(out, "    goto ");
            print_operand(out, &q->res);
            fprintf(out, "\n");
            break;
        case IR_IF_FALSE:
            fprintf(out, "    if_false ");
            print_operand(out, &q->arg1);
            fprintf(out, " goto ");
            print_operand(out, &q->res);
            fprintf(out, "\n");
            break;
        case IR_PARAM:
            fprintf(out, "    param ");
            print_operand(out, &q->arg1);
            fprintf(out, "\n");
            break;
        case IR_CALL:
            fprintf(out, "    ");
            if (q->res.kind != OPD_NONE) {
                print_operand(out, &q->res);
                fprintf(out, " = ");
            }
            fprintf(out, "call %s, %d\n", q->arg1.name, q->arg2.value);
            break;
        case IR_RETURN:
            fprintf(out, "    return");
            if (q->arg1.kind != OPD_NONE) {
                fprintf(out, " ");
                print_operand(out, &q->arg1);
            }
            fprintf(out, "\n");
            break;
        case IR_COMMENT:
            fprintf(out, "; %s\n", q->text);
            break;
    }
}

void ir_print(const IRProgram *p, FILE *out) {
    if (!p) return;
    for (const IRQuad *q = p->head; q; q = q->next) print_quad(out, q);
}

static void free_operand(Operand *o) {
    free(o->name);
    o->name = NULL;
}

void ir_free(IRProgram *p) {
    if (!p) return;
    IRQuad *q = p->head;
    while (q) {
        IRQuad *next = q->next;
        free_operand(&q->arg1);
        free_operand(&q->arg2);
        free_operand(&q->res);
        free(q->text);
        free(q);
        q = next;
    }
    free(p);
}
