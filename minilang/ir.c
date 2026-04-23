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
    if (e->kind == EXPR_VAR) return strdup(e->sval);

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
        emit("ASSIGN", s->name, src, NULL);
    }
    else if (s->kind == STMT_IF) {
        char *cond   = gen_expr(s->expr);
        char *l_else = new_label();
        char *l_end  = new_label();
        emit("JUMPF",  l_else, cond,   NULL);
        gen_stmtlist(s->then_body);
        emit("JUMP",   l_end,  NULL,   NULL);
        emit("LABEL",  l_else, NULL,   NULL);
        gen_stmtlist(s->else_body);
        emit("LABEL",  l_end,  NULL,   NULL);
    }
    else if (s->kind == STMT_LOOP) {
        char *start   = gen_expr(s->start);
        char *l_start = new_label();
        char *l_end   = new_label();
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
    }
    else if (s->kind == STMT_PRINT) {
        char *val = gen_expr(s->expr);
        emit("PRINT", NULL, val, NULL);
    }
}

void generate_ir(StmtList *stmts) {
    gen_stmtlist(stmts);

    printf("\n=== IR ===\n");
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
