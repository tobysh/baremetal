#include <stdio.h>

int digit_sum(int n) {
    int total = 0;
    while (n > 0) {
        total += n % 10;
        n /= 10;
    }
    return total;
}

int count_divisible(int lo, int hi, int d) {
    int count = 0;
    for (int x = lo; x <= hi; x++) {
        if (x % d == 0) {
            count++;
        }
    }
    return count;
}

int is_prime(int n) {
    if (n < 2) {
        return 0;
    }
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

int main(void) {
    printf("digit_sum(1234) = %d\n", digit_sum(1234));
    printf("count_divisible(1, 10, 3) = %d\n", count_divisible(1, 10, 3));
    printf("is_prime(7) = %d\n", is_prime(7));
    return 0;
}
