int main(void) {
    CHECK("sum_digits(0) is 0", sum_digits(0) == 0, "got %d", sum_digits(0));
    CHECK("sum_digits(7) is 7", sum_digits(7) == 7, "got %d", sum_digits(7));
    CHECK("sum_digits(472) is 13", sum_digits(472) == 13, "got %d, expected 13", sum_digits(472));
    CHECK("sum_digits(1000) is 1", sum_digits(1000) == 1, "got %d", sum_digits(1000));
    CHECK("sum_digits(999999999) is 81", sum_digits(999999999) == 81, "got %d", sum_digits(999999999));

    int a[] = {1, 2, 3, 4, 5};
    CHECK("subsets of {1,2,3,4,5} summing to 5 (5, 1+4, 2+3)", count_subsets(a, 5, 5) == 3, "got %d, expected 3", count_subsets(a, 5, 5));
    CHECK("subsets of {1,2,3,4,5} summing to 15 (all of them)", count_subsets(a, 5, 15) == 1, "got %d, expected 1", count_subsets(a, 5, 15));
    CHECK("no subset of {1,2,3,4,5} sums to 16", count_subsets(a, 5, 16) == 0, "got %d, expected 0", count_subsets(a, 5, 16));
    CHECK("the empty subset counts for target 0", count_subsets(a, 5, 0) == 1, "got %d, expected 1", count_subsets(a, 5, 0));
    int d[] = {1, 1};
    CHECK("duplicates count separately", count_subsets(d, 2, 1) == 2, "got %d, expected 2", count_subsets(d, 2, 1));
    int z[] = {3, -3, 2, -2};
    CHECK("negative numbers work (subsets summing to 0)", count_subsets(z, 4, 0) == 4, "got %d, expected 4", count_subsets(z, 4, 0));
    CHECK("n = 0 has one subset (the empty one) for target 0", count_subsets(a, 0, 0) == 1, "got %d, expected 1", count_subsets(a, 0, 0));
    CHECK("n = 0 has none for target 3", count_subsets(a, 0, 3) == 0, "got %d, expected 0", count_subsets(a, 0, 3));
    int big[16];
    for (int i = 0; i < 16; i++) big[i] = 1;
    CHECK("sixteen 1s: choosing 8 of them is 12870 ways", count_subsets(big, 16, 8) == 12870, "got %d, expected 12870", count_subsets(big, 16, 8));

    int want[] = {1, 0, 0, 2, 10, 4, 40, 92};
    for (int n = 1; n <= 8; n++) {
        int got = count_queens(n);
        CHECK(n == 1 ? "count_queens(1) is 1" : n == 2 ? "count_queens(2) is 0" : n == 3 ? "count_queens(3) is 0" : n == 4 ? "count_queens(4) is 2" : n == 5 ? "count_queens(5) is 10" : n == 6 ? "count_queens(6) is 4" : n == 7 ? "count_queens(7) is 40" : "count_queens(8) is 92",
              got == want[n - 1], "got %d, expected %d", got, want[n - 1]);
    }
    return 0;
}
