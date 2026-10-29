#include <stdio.h>

int main(void) {
    int number;
    if (scanf("%d", &number) != 1) {
        return 1;
    }
    printf("%d\n", number * 2);
    return 0;
}
