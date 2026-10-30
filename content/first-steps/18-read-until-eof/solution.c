#include <stdio.h>

int main(void) {
    int count = 0, sum = 0, max = 0, x;
    while (scanf("%d", &x) == 1) {
        count++;
        sum += x;
        if (count == 1 || x > max) max = x;
    }
    printf("count: %d\n", count);
    printf("sum: %d\n", sum);
    if (count == 0) printf("max: none\n");
    else printf("max: %d\n", max);
    return 0;
}
