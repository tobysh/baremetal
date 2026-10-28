#include <stdio.h>

int main(void) {
    int n;
    scanf("%d", &n);

    int count = 0, sum = 0, reversed = 0;
    while (n > 0) {
        int digit = n % 10;
        count++;
        sum += digit;
        reversed = reversed * 10 + digit;
        n /= 10;
    }

    printf("digits: %d\n", count);
    printf("sum: %d\n", sum);
    printf("reversed: %d\n", reversed);
    return 0;
}
