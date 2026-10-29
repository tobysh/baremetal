#include <stdio.h>
#include <stdlib.h>

static int place(int n, const char *const *board, int row, unsigned cols, unsigned d1, unsigned d2) {
    if (row == n) return 1;
    int total = 0;
    for (int c = 0; c < n; c++) {
        if (board[row][c] == '#') continue;
        unsigned a = 1u << c, b = 1u << (row + c), d = 1u << (row - c + n - 1);
        if ((cols & a) || (d1 & b) || (d2 & d)) continue;
        total += place(n, board, row + 1, cols | a, d1 | b, d2 | d);
    }
    return total;
}

int count_queens_on(int n, const char *const *board) {
    return place(n, board, 0, 0, 0, 0);
}

// Optional: try it with Run.
int main(void) {
    const char *const board[] = { "....", "..#.", "....", "...." };
    printf("%d ways\n", count_queens_on(4, board));
    return 0;
}
