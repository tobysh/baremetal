#include <stdio.h>

int main(void) {
    for (int row = 1; row <= 4; row++) {
        for (int col = 1; col <= row; col++) {
            printf("*");
        }
        printf("\n");
    }

    for (int row = 1; row <= 3; row++) {
        for (int col = 1; col <= 3; col++) {
            if (col > 1) printf(" ");
            printf("%d", row * col);
        }
        printf("\n");
    }
    return 0;
}
