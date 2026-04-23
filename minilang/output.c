#include <stdio.h>

int main() {
    int x = 10;
    int y = 20;
    int t0 = y * 2;
    int t1 = x + t0;
    int z = t1;
    int t2 = (z > 30);
    if (!t2) goto L0;
    int result = 1;
    goto L1;
L0:;
    int result = 0;
L1:;
    return 0;
}
