#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 1;
    int total = 0;
    for (int i = 1; i <= n; i++) {
        if (i % 3 == 0) continue;
        if (total + i > 30) break;
        total += i;
        printf("%d\n", i);
    }
    printf("total: %d\n", total);
    return 0;
}
