#include <stdio.h>
#include <string.h>
#include "ir.h"

void emit_c(IRInstr *ir) {
    FILE *f = fopen("output.c", "w");
    fprintf(f, "#include <stdio.h>\n\nint main() {\n");

    for (IRInstr *i = ir; i; i = i->next) {
        if      (!strcmp(i->op,"ASSIGN")) fprintf(f, "    int %s = %s;\n",           i->dest, i->src1);
        else if (!strcmp(i->op,"ADD"))    fprintf(f, "    int %s = %s + %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"SUB"))    fprintf(f, "    int %s = %s - %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"MUL"))    fprintf(f, "    int %s = %s * %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"DIV"))    fprintf(f, "    int %s = %s / %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"GT"))     fprintf(f, "    int %s = (%s > %s);\n",    i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"LT"))     fprintf(f, "    int %s = (%s < %s);\n",    i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"GTE"))    fprintf(f, "    int %s = (%s >= %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"LTE"))    fprintf(f, "    int %s = (%s <= %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"EQ"))     fprintf(f, "    int %s = (%s == %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"NEQ"))    fprintf(f, "    int %s = (%s != %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"JUMPF"))  fprintf(f, "    if (!%s) goto %s;\n",      i->src1, i->dest);
        else if (!strcmp(i->op,"JUMP"))   fprintf(f, "    goto %s;\n",               i->dest);
        else if (!strcmp(i->op,"LABEL"))  fprintf(f, "%s:;\n",                       i->dest);
    }

    fprintf(f, "    return 0;\n}\n");
    fclose(f);
    printf("C output written to output.c\n");
}
