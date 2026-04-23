#include <stdio.h>

int main() {
    int total = 0;
    int i = 0;
    int t0 = 0;
    int t1 = 0;
    int t2 = 0;
    int t4 = 0;

    total = 0;
    i = 1;
L0:;
    t0 = (i <= 10);
    if (!t0) goto L1;
    t1 = (i > 5);
    if (!t1) goto L2;
    t2 = total + i;
    total = t2;
    goto L3;
L2:;
    total = total;
L3:;
    t4 = i + 1;
    i = t4;
    goto L0;
L1:;
    printf("%d\n", total);
    return 0;
}
