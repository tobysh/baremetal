#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 1;
    int total = 0;
    // Loop from 1 to n. Skip multiples of 3, stop before the total passes 30.

    printf("total: %d\n", total);
    return 0;
}
