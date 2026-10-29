#include <stdio.h>

int main(void) {
    int a, b;
    if (scanf("%d %d", &a, &b) != 2) return 1;
    printf("sum: %d\n", a + b);
    if (a > b) printf("larger: %d\n", a);
    else if (b > a) printf("larger: %d\n", b);
    else printf("larger: equal\n");
    return 0;
}
