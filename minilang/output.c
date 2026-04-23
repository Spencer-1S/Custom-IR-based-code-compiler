#include <stdio.h>

int main() {
    int x = 0;
    int y = 0;
    int t0 = 0;
    int t1 = 0;
    int z = 0;
    int result = 0;
    int t2 = 0;

    x = 10;
    y = 20;
    t0 = y * 2;
    t1 = x + t0;
    z = t1;
    result = 0;
    t2 = (z > 30);
    if (!t2) goto L0;
    result = 1;
    goto L1;
L0:;
    result = 0;
L1:;
    printf("%d\n", result);
    return 0;
}
