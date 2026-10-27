#include <stdio.h>
#include <stdlib.h>

int sum_digits(int n) {
    if (n < 10) {
        return n;
    }
    return n % 10 + sum_digits(n / 10);
}

static int subsets_from(const int *a, int n, int index, int remaining) {
    if (index == n) {
        return remaining == 0 ? 1 : 0;
    }
    int taken = subsets_from(a, n, index + 1, remaining - a[index]);
    int left = subsets_from(a, n, index + 1, remaining);
    return taken + left;
}

int count_subsets(const int *a, int n, int target) {
    return subsets_from(a, n, 0, target);
}

static int safe(const int *cols, int row, int c) {
    for (int r = 0; r < row; r++) {
        if (cols[r] == c || abs(cols[r] - c) == row - r) {
            return 0;
        }
    }
    return 1;
}

static int place(int n, int row, int *cols) {
    if (row == n) {
        return 1;
    }
    int total = 0;
    for (int c = 0; c < n; c++) {
        if (safe(cols, row, c)) {
            cols[row] = c;
            total += place(n, row + 1, cols);
        }
    }
    return total;
}

int count_queens(int n) {
    int cols[8];
    if (n < 1 || n > 8) {
        return 0;
    }
    return place(n, 0, cols);
}

int main(void) {
    printf("%d\n", sum_digits(472));
    return 0;
}
