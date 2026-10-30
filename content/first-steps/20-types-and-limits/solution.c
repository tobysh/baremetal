#include <limits.h>
#include <stdio.h>

int main(void) {
    long long a, b;
    if (scanf("%lld %lld", &a, &b) != 2) return 1;
    long long sum = a + b;
    printf("int: %d bytes\n", (int)sizeof(int));
    printf("long long: %d bytes\n", (int)sizeof(long long));
    printf("sum: %lld\n", sum);
    printf("fits in int: %s\n", (sum >= INT_MIN && sum <= INT_MAX) ? "yes" : "no");
    return 0;
}
