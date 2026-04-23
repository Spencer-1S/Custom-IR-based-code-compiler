#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"

static IRInstr *ir_head = NULL, *ir_tail = NULL;
static int temp_count  = 0;
static int label_count = 0;

static char *new_temp() {
    char *t = malloc(16);
    sprintf(t, "t%d", temp_count++);
    return t;
}

static char *new_label() {
    char *l = malloc(16);
    sprintf(l, "L%d", label_count++);
    return l;
}

static void emit(char *op, char *dest, char *src1, char *src2) {
    IRInstr *i = malloc(sizeof(IRInstr));
    i->op   = strdup(op);
    i->dest = dest ? strdup(dest) : NULL;
    i->src1 = src1 ? strdup(src1) : NULL;
    i->src2 = src2 ? strdup(src2) : NULL;
    i->next = NULL;
    if (!ir_head) ir_head = ir_tail = i;
    else { ir_tail->next = i; ir_tail = i; }
}

/* ── IR Optimization Pipeline (middle-end) ── */
typedef struct Binding {
    char *name;
    char *value;
    enum { BIND_CONST, BIND_ALIAS } kind;
    struct Binding *next;
} Binding;

static Binding *env_find(Binding *env, const char *name);

static int alias_chain_contains(Binding *env, const char *start, const char *target, int depth) {
    if (!start || !target || depth <= 0) return 0;
    if (!strcmp(start, target)) return 1;
    Binding *b = env_find(env, start);
    if (!b || b->kind != BIND_ALIAS || !b->value) return 0;
    return alias_chain_contains(env, b->value, target, depth - 1);
}

static int binding_depends_on(Binding *env, Binding *b, const char *target) {
    if (!b || b->kind != BIND_ALIAS || !b->value) return 0;
    return alias_chain_contains(env, b->value, target, 32);
}

/*
 * Copy propagation is only correct if we kill aliases that depend on a variable
 * when that variable is redefined later in the same basic block.
 */
static void env_invalidate_dependents(Binding **env, const char *target) {
    if (!env || !*env || !target) return;
    int changed;
    do {
        changed = 0;
        Binding *prev = NULL;
        for (Binding *b = *env; b; ) {
            Binding *next = b->next;
            int remove = (!strcmp(b->name, target)) || binding_depends_on(*env, b, target);
            if (remove) {
                if (prev) prev->next = next;
                else *env = next;
                changed = 1;
            } else {
                prev = b;
            }
            b = next;
        }
    } while (changed);
}

static int is_int_literal(const char *s, long *out) {
    if (!s || !*s) return 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end && *end == '\0') {
        if (out) *out = v;
        return 1;
    }
    return 0;
}

static int is_temp_name(const char *s) {
    if (!s || s[0] != 't') return 0;
    for (int i = 1; s[i]; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
    }
    return 1;
}

static int is_var_operand(const char *s) {
    long tmp;
    return s && !is_int_literal(s, &tmp);
}

static Binding *env_find(Binding *env, const char *name) {
    for (Binding *b = env; b; b = b->next) {
        if (!strcmp(b->name, name)) return b;
    }
    return NULL;
}

static void env_set(Binding **env, const char *name, int kind, const char *value) {
    Binding *b = env_find(*env, name);
    if (!b) {
        b = malloc(sizeof(Binding));
        b->name = strdup(name);
        b->next = *env;
        *env = b;
    }
    b->kind = kind;
    b->value = value ? strdup(value) : NULL;
}

static const char *env_resolve(Binding *env, const char *name, int depth) {
    if (!name || depth <= 0) return name;
    Binding *b = env_find(env, name);
    if (!b) return name;
    if (b->kind == BIND_CONST) return b->value;
    if (b->kind == BIND_ALIAS) return env_resolve(env, b->value, depth - 1);
    return name;
}

static void instr_set(IRInstr *i, const char *op, const char *dest, const char *src1, const char *src2) {
    i->op = strdup(op);
    i->dest = dest ? strdup(dest) : NULL;
    i->src1 = src1 ? strdup(src1) : NULL;
    i->src2 = src2 ? strdup(src2) : NULL;
}

static void instr_replace_src(char **field, const char *new_val) {
    if (!field || !new_val) return;
    if (*field && !strcmp(*field, new_val)) return;
    *field = strdup(new_val);
}

static int is_binary_op(const char *op) {
    return !strcmp(op, "ADD") || !strcmp(op, "SUB") || !strcmp(op, "MUL") || !strcmp(op, "DIV") ||
           !strcmp(op, "GT")  || !strcmp(op, "LT")  || !strcmp(op, "GTE") || !strcmp(op, "LTE") ||
           !strcmp(op, "EQ")  || !strcmp(op, "NEQ");
}

static int is_control_op(const char *op) {
    return !strcmp(op, "JUMP") || !strcmp(op, "JUMPF") || !strcmp(op, "LABEL") || !strcmp(op, "PRINT");
}

static void used_add(char **used, int *used_n, int used_cap, const char *name) {
    if (!name) return;
    for (int i = 0; i < *used_n; i++) {
        if (!strcmp(used[i], name)) return;
    }
    if (*used_n < used_cap) used[(*used_n)++] = (char *)name;
}

static int used_has(char **used, int used_n, const char *name) {
    if (!name) return 0;
    for (int i = 0; i < used_n; i++) {
        if (!strcmp(used[i], name)) return 1;
    }
    return 0;
}

static void used_remove(char **used, int *used_n, const char *name) {
    if (!name) return;
    for (int i = 0; i < *used_n; i++) {
        if (!strcmp(used[i], name)) {
            used[i] = used[--(*used_n)];
            return;
        }
    }
}

static void optimize_block(IRInstr **nodes, int n, int *keep) {
    Binding *env = NULL;

    /* Forward: local constant/copy propagation + folding */
    for (int idx = 0; idx < n; idx++) {
        IRInstr *i = nodes[idx];
        keep[idx] = 1;

        if (!strcmp(i->op, "LABEL")) {
            /* start-of-block marker, no change */
            continue;
        }

        /* Substitute operands via env */
        if (i->src1 && is_var_operand(i->src1)) {
            const char *r = env_resolve(env, i->src1, 16);
            if (r && strcmp(r, i->src1)) instr_replace_src(&i->src1, r);
        }
        if (i->src2 && is_var_operand(i->src2)) {
            const char *r = env_resolve(env, i->src2, 16);
            if (r && strcmp(r, i->src2)) instr_replace_src(&i->src2, r);
        }

        if (!strcmp(i->op, "ASSIGN") && i->dest) {
            env_invalidate_dependents(&env, i->dest);
            /* Track const/copy */
            if (i->src1) {
                long v;
                if (is_int_literal(i->src1, &v)) {
                    env_set(&env, i->dest, BIND_CONST, i->src1);
                } else {
                    env_set(&env, i->dest, BIND_ALIAS, i->src1);
                }
            } else {
                /* unknown value */
            }
            continue;
        }

        if (is_binary_op(i->op) && i->dest) {
            env_invalidate_dependents(&env, i->dest);
            long a, b;
            int a_is = is_int_literal(i->src1, &a);
            int b_is = is_int_literal(i->src2, &b);

            /* Algebraic simplifications */
            if (!strcmp(i->op, "ADD")) {
                if (a_is && a == 0) { instr_set(i, "ASSIGN", i->dest, i->src2, NULL); env_set(&env, i->dest, b_is ? BIND_CONST : BIND_ALIAS, i->src2); continue; }
                if (b_is && b == 0) { instr_set(i, "ASSIGN", i->dest, i->src1, NULL); env_set(&env, i->dest, a_is ? BIND_CONST : BIND_ALIAS, i->src1); continue; }
            } else if (!strcmp(i->op, "SUB")) {
                if (b_is && b == 0) { instr_set(i, "ASSIGN", i->dest, i->src1, NULL); env_set(&env, i->dest, a_is ? BIND_CONST : BIND_ALIAS, i->src1); continue; }
            } else if (!strcmp(i->op, "MUL")) {
                if ((a_is && a == 0) || (b_is && b == 0)) { instr_set(i, "ASSIGN", i->dest, "0", NULL); env_set(&env, i->dest, BIND_CONST, "0"); continue; }
                if (a_is && a == 1) { instr_set(i, "ASSIGN", i->dest, i->src2, NULL); env_set(&env, i->dest, b_is ? BIND_CONST : BIND_ALIAS, i->src2); continue; }
                if (b_is && b == 1) { instr_set(i, "ASSIGN", i->dest, i->src1, NULL); env_set(&env, i->dest, a_is ? BIND_CONST : BIND_ALIAS, i->src1); continue; }
            } else if (!strcmp(i->op, "DIV")) {
                if (a_is && a == 0 && !(b_is && b == 0)) { instr_set(i, "ASSIGN", i->dest, "0", NULL); env_set(&env, i->dest, BIND_CONST, "0"); continue; }
                if (b_is && b == 1) { instr_set(i, "ASSIGN", i->dest, i->src1, NULL); env_set(&env, i->dest, a_is ? BIND_CONST : BIND_ALIAS, i->src1); continue; }
            }

            /* Constant folding */
            if (a_is && b_is) {
                long res = 0;
                int ok = 1;
                if (!strcmp(i->op, "ADD")) res = a + b;
                else if (!strcmp(i->op, "SUB")) res = a - b;
                else if (!strcmp(i->op, "MUL")) res = a * b;
                else if (!strcmp(i->op, "DIV")) { if (b == 0) ok = 0; else res = a / b; }
                else if (!strcmp(i->op, "GT"))  res = (a >  b);
                else if (!strcmp(i->op, "LT"))  res = (a <  b);
                else if (!strcmp(i->op, "GTE")) res = (a >= b);
                else if (!strcmp(i->op, "LTE")) res = (a <= b);
                else if (!strcmp(i->op, "EQ"))  res = (a == b);
                else if (!strcmp(i->op, "NEQ")) res = (a != b);
                if (ok) {
                    char buf[64];
                    sprintf(buf, "%ld", res);
                    instr_set(i, "ASSIGN", i->dest, buf, NULL);
                    env_set(&env, i->dest, BIND_CONST, buf);
                    continue;
                }
            }

            /* Unknown value now */
            continue;
        }

        if (!strcmp(i->op, "JUMPF")) {
            /* If condition becomes constant, simplify */
            long cv;
            if (is_int_literal(i->src1, &cv)) {
                if (cv == 0) {
                    instr_set(i, "JUMP", i->dest, NULL, NULL);
                } else {
                    keep[idx] = 0; /* never jumps */
                }
            }
            continue;
        }

        if (!strcmp(i->op, "JUMP") || !strcmp(i->op, "PRINT")) {
            continue;
        }
    }

    /* Backward: dead-temp elimination within the block (conservative) */
    char *used[2048];
    int used_n = 0;
    for (int idx = n - 1; idx >= 0; idx--) {
        if (!keep[idx]) continue;
        IRInstr *i = nodes[idx];
        const char *op = i->op;

        /* Determine uses */
        if (!strcmp(op, "PRINT")) {
            if (is_var_operand(i->src1)) used_add(used, &used_n, 2048, i->src1);
            continue;
        }
        if (!strcmp(op, "JUMPF")) {
            if (is_var_operand(i->src1)) used_add(used, &used_n, 2048, i->src1);
            continue;
        }
        if (!strcmp(op, "JUMP") || !strcmp(op, "LABEL")) {
            continue;
        }

        if (!strcmp(op, "ASSIGN")) {
            if (is_var_operand(i->src1)) used_add(used, &used_n, 2048, i->src1);
        } else if (is_binary_op(op)) {
            if (is_var_operand(i->src1)) used_add(used, &used_n, 2048, i->src1);
            if (is_var_operand(i->src2)) used_add(used, &used_n, 2048, i->src2);
        }

        /* Determine def */
        if (i->dest && !is_control_op(op)) {
            if (is_temp_name(i->dest) && !used_has(used, used_n, i->dest)) {
                keep[idx] = 0;
                continue;
            }
            used_remove(used, &used_n, i->dest);
        }
    }
}

static IRInstr *optimize_ir(IRInstr *head) {
    IRInstr *out_head = NULL;
    IRInstr *out_tail = NULL;

    IRInstr *cur = head;
    while (cur) {
        /* Collect a basic block */
        IRInstr *block_nodes[4096];
        int n = 0;
        IRInstr *start = cur;

        /* If label begins block, include it */
        if (cur && !strcmp(cur->op, "LABEL")) {
            block_nodes[n++] = cur;
            cur = cur->next;
        }

        while (cur) {
            if (!strcmp(cur->op, "LABEL")) break;
            block_nodes[n++] = cur;
            if (!strcmp(cur->op, "JUMP") || !strcmp(cur->op, "JUMPF")) {
                cur = cur->next;
                break;
            }
            cur = cur->next;
        }

        (void)start;
        int keep[4096];
        optimize_block(block_nodes, n, keep);

        /* Append kept instructions to new list */
        for (int i = 0; i < n; i++) {
            if (!keep[i]) continue;
            IRInstr *node = block_nodes[i];
            node->next = NULL;
            if (!out_head) out_head = out_tail = node;
            else { out_tail->next = node; out_tail = node; }
        }
    }

    /* Pass: remove redundant jumps and unreachable code after unconditional jump */
    IRInstr *prev = NULL;
    IRInstr *i = out_head;
    while (i) {
        if (!strcmp(i->op, "JUMP")) {
            /* Remove jump-to-next-label */
            IRInstr *nxt = i->next;
            if (nxt && !strcmp(nxt->op, "LABEL") && i->dest && nxt->dest && !strcmp(i->dest, nxt->dest)) {
                if (prev) prev->next = nxt;
                else out_head = nxt;
                i = nxt;
                continue;
            }

            /* Remove unreachable instructions until next LABEL */
            IRInstr *scan = i->next;
            while (scan && strcmp(scan->op, "LABEL")) {
                scan = scan->next;
            }
            i->next = scan;
        }
        prev = i;
        i = i->next;
    }

    return out_head;
}

/* ── Semantic Scoping ── */
typedef struct Symbol {
    char *name;
    struct Symbol *next;
} Symbol;

typedef struct Scope {
    Symbol *syms;
    struct Scope *parent;
} Scope;

static Scope *current_scope = NULL;

static void push_scope() {
    Scope *s = malloc(sizeof(Scope));
    s->syms = NULL;
    s->parent = current_scope;
    current_scope = s;
}

static void pop_scope() {
    if (current_scope) {
        current_scope = current_scope->parent;
    }
}

static void add_symbol(char *name) {
    if (!current_scope) return;
    for (Symbol *sym = current_scope->syms; sym; sym = sym->next) {
        if (!strcmp(sym->name, name)) return;
    }
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->next = current_scope->syms;
    current_scope->syms = sym;
}

static int check_symbol(char *name) {
    for (Scope *s = current_scope; s; s = s->parent) {
        for (Symbol *sym = s->syms; sym; sym = sym->next) {
            if (!strcmp(sym->name, name)) return 1;
        }
    }
    return 0;
}

/* ── AST node builders ── */
Expr *make_num(int val) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = EXPR_NUM; e->ival = val;
    return e;
}
Expr *make_var(char *name) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = EXPR_VAR; e->sval = strdup(name);
    return e;
}
Expr *make_binop(char *op, Expr *l, Expr *r) {
    Expr *e = malloc(sizeof(Expr));
    e->kind = EXPR_BINOP; e->op = op;
    e->left = l; e->right = r;
    return e;
}
Stmt *make_assign(char *name, Expr *val) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_ASSIGN;
    s->name = strdup(name); s->expr = val;
    s->next = NULL;
    return s;
}
Stmt *make_set(char *name, Expr *val) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_SET;
    s->name = strdup(name);
    s->expr = val;
    s->next = NULL;
    return s;
}
Stmt *make_if(Expr *cond, StmtList *then, StmtList *els) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_IF; s->expr = cond;
    s->then_body = then; s->else_body = els;
    s->next = NULL;
    return s;
}
Stmt *make_loop(char *var, Expr *start, Expr *end, StmtList *body) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind  = STMT_LOOP;
    s->name  = strdup(var);
    s->start = start; s->end = end; s->body = body;
    s->next  = NULL;
    return s;
}
Stmt *make_print(Expr *val) {
    Stmt *s = malloc(sizeof(Stmt));
    s->kind = STMT_PRINT;
    s->expr = val;
    s->next = NULL;
    return s;
}
StmtList *make_stmtlist(Stmt *s) {
    StmtList *l = malloc(sizeof(StmtList));
    l->head = l->tail = s;
    return l;
}
StmtList *append_stmt(StmtList *list, Stmt *s) {
    if (!list) return make_stmtlist(s);
    list->tail->next = s; list->tail = s;
    return list;
}

/* ── Expression codegen ── */
static char *gen_expr(Expr *e) {
    if (e->kind == EXPR_NUM) {
        char *buf = malloc(32);
        sprintf(buf, "%d", e->ival);
        return buf;
    }
    if (e->kind == EXPR_VAR) {
        if (!check_symbol(e->sval)) {
            fprintf(stderr, "Semantic Error: Variable '%s' is accessed outside of its scope or undeclared.\n", e->sval);
            exit(1);
        }
        return strdup(e->sval);
    }

    char *l = gen_expr(e->left);
    char *r = gen_expr(e->right);
    char *t = new_temp();
    char *op = NULL;

    if      (!strcmp(e->op, "+"))  op = "ADD";
    else if (!strcmp(e->op, "-"))  op = "SUB";
    else if (!strcmp(e->op, "*"))  op = "MUL";
    else if (!strcmp(e->op, "/"))  op = "DIV";
    else if (!strcmp(e->op, ">"))  op = "GT";
    else if (!strcmp(e->op, "<"))  op = "LT";
    else if (!strcmp(e->op, ">=")) op = "GTE";
    else if (!strcmp(e->op, "<=")) op = "LTE";
    else if (!strcmp(e->op, "==")) op = "EQ";
    else if (!strcmp(e->op, "!=")) op = "NEQ";

    emit(op, t, l, r);
    return t;
}

/* ── Statement codegen ── */
static void gen_stmt(Stmt *s);

static void gen_stmtlist(StmtList *list) {
    if (!list) return;
    for (Stmt *s = list->head; s; s = s->next)
        gen_stmt(s);
}

static void gen_stmt(Stmt *s) {
    if (s->kind == STMT_ASSIGN) {
        char *src = gen_expr(s->expr);
        add_symbol(s->name);
        emit("ASSIGN", s->name, src, NULL);
    }
    else if (s->kind == STMT_SET) {
        if (!check_symbol(s->name)) {
            fprintf(stderr, "Semantic Error: Assignment to undeclared variable '%s'.\n", s->name);
            exit(1);
        }
        char *src = gen_expr(s->expr);
        emit("ASSIGN", s->name, src, NULL);
    }
    else if (s->kind == STMT_IF) {
        char *cond   = gen_expr(s->expr);
        char *l_else = new_label();
        char *l_end  = new_label();
        emit("JUMPF",  l_else, cond,   NULL);
        
        push_scope();
        gen_stmtlist(s->then_body);
        pop_scope();
        
        emit("JUMP",   l_end,  NULL,   NULL);
        emit("LABEL",  l_else, NULL,   NULL);
        
        push_scope();
        gen_stmtlist(s->else_body);
        pop_scope();
        
        emit("LABEL",  l_end,  NULL,   NULL);
    }
    else if (s->kind == STMT_LOOP) {
        char *start   = gen_expr(s->start);
        char *l_start = new_label();
        char *l_end   = new_label();
        
        push_scope();
        add_symbol(s->name);
        emit("ASSIGN", s->name,   start,   NULL);
        emit("LABEL",  l_start,   NULL,    NULL);
        char *limit = gen_expr(s->end);
        char *cond  = new_temp();
        emit("LTE",    cond,     s->name,  limit);
        emit("JUMPF",  l_end,    cond,     NULL);
        
        gen_stmtlist(s->body);
        
        char *inc = new_temp();
        emit("ADD",    inc,      s->name,  "1");
        emit("ASSIGN", s->name,  inc,      NULL);
        emit("JUMP",   l_start,  NULL,     NULL);
        emit("LABEL",  l_end,    NULL,     NULL);
        
        pop_scope();
    }
    else if (s->kind == STMT_PRINT) {
        char *val = gen_expr(s->expr);
        emit("PRINT", NULL, val, NULL);
    }
}

void generate_ir(StmtList *stmts) {
    push_scope(); // Global scope
    gen_stmtlist(stmts);
    pop_scope();

    printf("\n=== IR ===\n");
    for (IRInstr *i = ir_head; i; i = i->next) {
        printf("%-8s", i->op);
        if (i->dest) printf(" %s",    i->dest);
        if (i->src1) printf(", %s",   i->src1);
        if (i->src2) printf(", %s",   i->src2);
        printf("\n");
    }

    ir_head = optimize_ir(ir_head);

    printf("\n=== Optimized IR ===\n");
    for (IRInstr *i = ir_head; i; i = i->next) {
        printf("%-8s", i->op);
        if (i->dest) printf(" %s",    i->dest);
        if (i->src1) printf(", %s",   i->src1);
        if (i->src2) printf(", %s",   i->src2);
        printf("\n");
    }

    emit_python(ir_head);
    emit_c(ir_head);
}

IRInstr *get_ir_list() { return ir_head; }
