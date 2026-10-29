#include <stdio.h>

#define SIZE 3

int grid_sum(int g[SIZE][SIZE]) {
    return 0;
}

int diagonal_sum(int g[SIZE][SIZE]) {
    return 0;
}

int row_max(int g[SIZE][SIZE], int row) {
    return 0;
}

void transpose(int g[SIZE][SIZE]) {
}

// Optional: try them with Run.
int main(void) {
    int g[SIZE][SIZE] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    printf("sum = %d (expected 45)\n", grid_sum(g));
    printf("diagonal = %d (expected 15)\n", diagonal_sum(g));
    printf("row 1 max = %d (expected 6)\n", row_max(g, 1));
    transpose(g);
    printf("g[0][1] = %d (expected 4)\n", g[0][1]);
    return 0;
}
