#include <stdio.h>

int main(void) {
    int rows = 3;
    int cols = 4;

    for (int r = 1; r <= rows; r++) {
        for (int c = 1; c <= cols; c++) {
            printf("%4d", r * c);
        }
        printf("\n");
    }
    return 0;
}
