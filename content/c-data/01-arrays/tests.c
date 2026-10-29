int main(void) {
    int a[5] = {70, 85, 90, 60, 100};
    CHECK("array_sum of 70 85 90 60 100 is 405", array_sum(a, 5) == 405, "got %d", array_sum(a, 5));
    int b[3] = {-4, 10, -1};
    CHECK("array_sum handles negatives", array_sum(b, 3) == 5, "got %d", array_sum(b, 3));
    int c[1] = {42};
    CHECK("array_sum of one element", array_sum(c, 1) == 42, "got %d", array_sum(c, 1));
    CHECK("array_sum of an empty array is 0", array_sum(a, 0) == 0, "got %d", array_sum(a, 0));
    CHECK("array_sum uses only the first n elements", array_sum(a, 2) == 155, "got %d", array_sum(a, 2));

    CHECK("array_max of 70 85 90 60 100 is 100", array_max(a, 5) == 100, "got %d", array_max(a, 5));
    int d[4] = {-9, -3, -7, -20};
    CHECK("array_max works when all are negative", array_max(d, 4) == -3, "got %d", array_max(d, 4));
    int e[4] = {9, 1, 2, 3};
    CHECK("array_max finds a maximum at the first index", array_max(e, 4) == 9, "got %d", array_max(e, 4));
    int f[4] = {1, 2, 3, 9};
    CHECK("array_max finds a maximum at the last index", array_max(f, 4) == 9, "got %d", array_max(f, 4));
    CHECK("array_max of one element", array_max(c, 1) == 42, "got %d", array_max(c, 1));
    CHECK("array_max uses only the first n elements", array_max(a, 2) == 85, "got %d", array_max(a, 2));

    CHECK("count_above(a, 80) is 3", count_above(a, 5, 80) == 3, "got %d", count_above(a, 5, 80));
    CHECK("count_above is strictly greater", count_above(a, 5, 90) == 1, "got %d", count_above(a, 5, 90));
    CHECK("count_above with a huge limit is 0", count_above(a, 5, 1000) == 0, "got %d", count_above(a, 5, 1000));
    CHECK("count_above with a tiny limit counts everything", count_above(d, 4, -100) == 4, "got %d", count_above(d, 4, -100));
    CHECK("count_above of an empty array is 0", count_above(a, 0, 0) == 0, "got %d", count_above(a, 0, 0));
    return 0;
}
