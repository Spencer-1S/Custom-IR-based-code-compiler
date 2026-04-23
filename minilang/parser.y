%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"

extern int  yylex();
extern FILE *yyin;
void yyerror(const char *msg) {
    fprintf(stderr, "Parse error: %s\n", msg);
    exit(1);
}
%}

%union {
    int       ival;
    char     *sval;
    Expr     *expr;
    Stmt     *stmt;
    StmtList *stmtlist;
}

%token LET IF ELSE LOOP FROM TO PRINT
%token LBRACE RBRACE ASSIGN
%token PLUS MINUS MUL DIV
%token GT LT GTE LTE EQ NEQ

%token <ival> NUMBER
%token <sval> IDENT

%type <expr>     expr
%type <stmt>     stmt
%type <stmtlist> stmtlist block

%left EQ NEQ
%left GT LT GTE LTE
%left PLUS MINUS
%left MUL DIV

%%

program:
    stmtlist    { generate_ir($1); }
    ;

stmtlist:
    stmtlist stmt   { $$ = append_stmt($1, $2); }
  | stmt            { $$ = make_stmtlist($1);   }
  ;

stmt:
    LET IDENT ASSIGN expr
        { $$ = make_assign($2, $4); }

  | IF expr LBRACE block RBRACE
        { $$ = make_if($2, $4, NULL); }

  | IF expr LBRACE block RBRACE ELSE LBRACE block RBRACE
        { $$ = make_if($2, $4, $8); }

  | LOOP IDENT FROM expr TO expr LBRACE block RBRACE
        { $$ = make_loop($2, $4, $6, $8); }

  | PRINT expr
        { $$ = make_print($2); }
  ;

block:
    stmtlist    { $$ = $1;   }
  | /* empty */ { $$ = NULL; }
  ;

expr:
    expr PLUS  expr  { $$ = make_binop("+",  $1, $3); }
  | expr MINUS expr  { $$ = make_binop("-",  $1, $3); }
  | expr MUL   expr  { $$ = make_binop("*",  $1, $3); }
  | expr DIV   expr  { $$ = make_binop("/",  $1, $3); }
  | expr GT    expr  { $$ = make_binop(">",  $1, $3); }
  | expr LT    expr  { $$ = make_binop("<",  $1, $3); }
  | expr GTE   expr  { $$ = make_binop(">=", $1, $3); }
  | expr LTE   expr  { $$ = make_binop("<=", $1, $3); }
  | expr EQ    expr  { $$ = make_binop("==", $1, $3); }
  | expr NEQ   expr  { $$ = make_binop("!=", $1, $3); }
  | NUMBER           { $$ = make_num($1);  }
  | IDENT            { $$ = make_var($1);  }
  ;

%%

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror(argv[1]);
            return 1;
        }
    }
    yyparse();
    return 0;
}
