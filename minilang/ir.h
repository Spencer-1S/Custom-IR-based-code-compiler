#ifndef IR_H
#define IR_H

/* ── AST node types ── */
typedef struct Expr {
    enum { EXPR_NUM, EXPR_VAR, EXPR_BINOP } kind;
    int          ival;
    char        *sval;
    char        *op;
    struct Expr *left, *right;
} Expr;

typedef struct Stmt {
    enum { STMT_ASSIGN, STMT_IF, STMT_LOOP } kind;
    char            *name;
    Expr            *expr;
    struct StmtList *then_body, *else_body, *body;
    Expr            *start, *end;
    struct Stmt     *next;
} Stmt;

typedef struct StmtList {
    Stmt *head;
    Stmt *tail;
} StmtList;

/* ── IR instruction ── */
typedef struct IRInstr {
    char            *op;
    char            *dest, *src1, *src2;
    struct IRInstr  *next;
} IRInstr;

/* ── AST builder functions ── */
Expr     *make_num(int val);
Expr     *make_var(char *name);
Expr     *make_binop(char *op, Expr *l, Expr *r);
Stmt     *make_assign(char *name, Expr *val);
Stmt     *make_if(Expr *cond, StmtList *then, StmtList *els);
Stmt     *make_loop(char *var, Expr *start, Expr *end, StmtList *body);
StmtList *make_stmtlist(Stmt *s);
StmtList *append_stmt(StmtList *list, Stmt *s);

/* ── IR generation and retrieval ── */
void     generate_ir(StmtList *stmts);
IRInstr *get_ir_list();

/* ── Backends ── */
void emit_python(IRInstr *ir);
void emit_c(IRInstr *ir);

#endif
