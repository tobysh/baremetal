#include <limits.h>
#include <stdio.h>

int main(void) {
    int a, b;
    if (scanf("%d %d", &a, &b) != 2) return 1;
    // This only works while the numbers fit in an int. Make it work for big ones.
    printf("int: %d bytes\n", (int)sizeof(int));
    printf("sum: %d\n", a + b);
    return 0;
}
