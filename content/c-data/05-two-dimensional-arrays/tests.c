int main(void) {
    int a[SIZE][SIZE] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    CHECK("grid_sum of 1..9 is 45", grid_sum(a) == 45, "got %d", grid_sum(a));
    int b[SIZE][SIZE] = {{-5, 0, 2}, {10, -1, 1}, {0, 0, -3}};
    CHECK("grid_sum handles negatives", grid_sum(b) == 4, "got %d", grid_sum(b));
    int z[SIZE][SIZE] = {{0}};
    CHECK("grid_sum of zeros is 0", grid_sum(z) == 0, "got %d", grid_sum(z));

    CHECK("diagonal_sum of 1..9 is 15", diagonal_sum(a) == 15, "got %d", diagonal_sum(a));
    CHECK("diagonal_sum ignores the other cells", diagonal_sum(b) == -9, "got %d", diagonal_sum(b));
    int d[SIZE][SIZE] = {{1, 100, 100}, {100, 2, 100}, {100, 100, 3}};
    CHECK("diagonal_sum of a ringed diagonal is 6", diagonal_sum(d) == 6, "got %d", diagonal_sum(d));

    CHECK("row_max of row 0", row_max(a, 0) == 3, "got %d", row_max(a, 0));
    CHECK("row_max of row 2", row_max(a, 2) == 9, "got %d", row_max(a, 2));
    CHECK("row_max with a maximum in the first column", row_max(b, 1) == 10, "got %d", row_max(b, 1));
    int n[SIZE][SIZE] = {{-4, -2, -9}, {-1, -8, -7}, {-6, -5, -3}};
    CHECK("row_max when the whole row is negative", row_max(n, 0) == -2 && row_max(n, 2) == -3, "got %d and %d", row_max(n, 0), row_max(n, 2));

    transpose(a);
    int ta[SIZE][SIZE] = {{1, 4, 7}, {2, 5, 8}, {3, 6, 9}};
    int ok = 1;
    for (int r = 0; r < SIZE; r++) for (int c = 0; c < SIZE; c++) if (a[r][c] != ta[r][c]) ok = 0;
    CHECK("transpose mirrors across the diagonal", ok, "got first row %d %d %d", a[0][0], a[0][1], a[0][2]);
    transpose(a);
    int orig[SIZE][SIZE] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    ok = 1;
    for (int r = 0; r < SIZE; r++) for (int c = 0; c < SIZE; c++) if (a[r][c] != orig[r][c]) ok = 0;
    CHECK("transposing twice gives the original", ok, "got first row %d %d %d", a[0][0], a[0][1], a[0][2]);
    transpose(b);
    CHECK("transpose swaps b[0][1] and b[1][0]", b[0][1] == 10 && b[1][0] == 0 && b[2][0] == 2 && b[0][2] == 0, "got %d %d %d %d", b[0][1], b[1][0], b[2][0], b[0][2]);
    return 0;
}
