#include <stdio.h>

#define SIZE 3

int grid_sum(int g[SIZE][SIZE]) {
    int total = 0;
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            total += g[r][c];
        }
    }
    return total;
}

int diagonal_sum(int g[SIZE][SIZE]) {
    int total = 0;
    for (int i = 0; i < SIZE; i++) {
        total += g[i][i];
    }
    return total;
}

int row_max(int g[SIZE][SIZE], int row) {
    int best = g[row][0];
    for (int c = 1; c < SIZE; c++) {
        if (g[row][c] > best) best = g[row][c];
    }
    return best;
}

void transpose(int g[SIZE][SIZE]) {
    for (int r = 0; r < SIZE; r++) {
        for (int c = r + 1; c < SIZE; c++) {
            int tmp = g[r][c];
            g[r][c] = g[c][r];
            g[c][r] = tmp;
        }
    }
}

int main(void) {
    int g[SIZE][SIZE] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    printf("sum = %d\n", grid_sum(g));
    printf("diagonal = %d\n", diagonal_sum(g));
    printf("row 1 max = %d\n", row_max(g, 1));
    transpose(g);
    printf("g[0][1] = %d\n", g[0][1]);
    return 0;
}
