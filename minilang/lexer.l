%{
#include "ir.h"
#include "parser.tab.h"
#pragma GCC diagnostic ignored "-Wunused-function"
%}

%%

[ \t\r]                  { /* skip whitespace */ }
\n                       { /* skip newlines   */ }
"#"[^\n]*                { /* skip comments   */ }

"let"                    { return LET;    }
"if"                     { return IF;     }
"else"                   { return ELSE;   }
"loop"                   { return LOOP;   }
"from"                   { return FROM;   }
"to"                     { return TO;     }

[0-9]+                   { yylval.ival = atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]*  { yylval.sval = strdup(yytext); return IDENT; }

"=="                     { return EQ;     }
"!="                     { return NEQ;    }
">="                     { return GTE;    }
"<="                     { return LTE;    }
">"                      { return GT;     }
"<"                      { return LT;     }
"="                      { return ASSIGN; }
"+"                      { return PLUS;   }
"-"                      { return MINUS;  }
"*"                      { return MUL;    }
"/"                      { return DIV;    }
"{"                      { return LBRACE; }
"}"                      { return RBRACE; }

.   { fprintf(stderr, "Unknown character: %s\n", yytext); }

%%

int yywrap() { return 1; }
