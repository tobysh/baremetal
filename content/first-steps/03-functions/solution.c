#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int square(int x) {
    return x * x;
}

int main(void) {
    printf("add(2, 3) = %d\n", add(2, 3));
    printf("square(4) = %d\n", square(4));
    return 0;
}
