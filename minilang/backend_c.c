#include <stdio.h>
#include <string.h>
#include "ir.h"

void emit_c(IRInstr *ir) {
    FILE *f = fopen("output.c", "w");
    fprintf(f, "#include <stdio.h>\n\nint main() {\n");

    /* PRE-PASS: Collect all unique variables to hoist them to the top */
    char *declared_vars[1000];
    int var_count = 0;

    for (IRInstr *i = ir; i; i = i->next) {
        if (!strcmp(i->op, "LABEL") || !strcmp(i->op, "JUMP") || 
            !strcmp(i->op, "JUMPF") || !strcmp(i->op, "PRINT")) {
            continue; // These instructions don't define variables
        }
        
        if (i->dest) {
            int found = 0;
            for (int k = 0; k < var_count; k++) {
                if (!strcmp(declared_vars[k], i->dest)) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                declared_vars[var_count++] = i->dest;
            }
        }
    }

    /* Print all declarations at the top of the C main function */
    for (int k = 0; k < var_count; k++) {
        fprintf(f, "    int %s = 0;\n", declared_vars[k]);
    }
    if (var_count > 0) fprintf(f, "\n");

    /* EMIT pass: Now format instructions without the 'int' prefix */
    for (IRInstr *i = ir; i; i = i->next) {
        if      (!strcmp(i->op,"ASSIGN")) fprintf(f, "    %s = %s;\n",           i->dest, i->src1);
        else if (!strcmp(i->op,"ADD"))    fprintf(f, "    %s = %s + %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"SUB"))    fprintf(f, "    %s = %s - %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"MUL"))    fprintf(f, "    %s = %s * %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"DIV"))    fprintf(f, "    %s = %s / %s;\n",      i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"GT"))     fprintf(f, "    %s = (%s > %s);\n",    i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"LT"))     fprintf(f, "    %s = (%s < %s);\n",    i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"GTE"))    fprintf(f, "    %s = (%s >= %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"LTE"))    fprintf(f, "    %s = (%s <= %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"EQ"))     fprintf(f, "    %s = (%s == %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"NEQ"))    fprintf(f, "    %s = (%s != %s);\n",   i->dest, i->src1, i->src2);
        else if (!strcmp(i->op,"JUMPF"))  fprintf(f, "    if (!%s) goto %s;\n",  i->src1, i->dest);
        else if (!strcmp(i->op,"JUMP"))   fprintf(f, "    goto %s;\n",           i->dest);
        else if (!strcmp(i->op,"LABEL"))  fprintf(f, "%s:;\n",                   i->dest);
        else if (!strcmp(i->op,"PRINT"))  fprintf(f, "    printf(\"%%d\\n\", %s);\n", i->src1);
    }

    fprintf(f, "    return 0;\n}\n");
    fclose(f);
    printf("C output written to output.c\n");
}
