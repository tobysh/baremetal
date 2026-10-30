int main(void) {
    int a[] = {4, 9, 2, 7, 5, -3, 12, 12, 0};
    int n = (int)(sizeof a / sizeof a[0]);

    CHECK("sum_between over the whole array is 48", sum_between(a, a + n) == 48, "got %d", sum_between(a, a + n));
    CHECK("sum_between of an empty range is 0", sum_between(a + 3, a + 3) == 0, "got %d", sum_between(a + 3, a + 3));
    CHECK("sum_between(a + 2, a + 5) is 2 + 7 + 5 = 14 (end excluded)", sum_between(a + 2, a + 5) == 14, "got %d", sum_between(a + 2, a + 5));
    CHECK("sum_between of one element is that element", sum_between(a + 5, a + 6) == -3, "got %d", sum_between(a + 5, a + 6));

    int b[] = {100, -1, -2};
    CHECK("sum_between works on another array", sum_between(b, b + 3) == 97, "got %d", sum_between(b, b + 3));

    const int *m = find_max(a, (size_t)n);
    CHECK("find_max points at the first 12 (index 6)", m == a + 6, "got index %ld", m ? (long)(m - a) : -1L);
    m = find_max(a, 4);
    CHECK("find_max of the first four elements points at index 1", m == a + 1, "got index %ld", m ? (long)(m - a) : -1L);
    m = find_max(a + 2, 3);
    CHECK("find_max(a + 2, 3) points at a[3] (7)", m == a + 3, "got index %ld", m ? (long)(m - a) : -1L);
    int neg[] = {-8, -3, -9};
    m = find_max(neg, 3);
    CHECK("find_max handles all-negative values", m == neg + 1, "got index %ld", m ? (long)(m - neg) : -1L);
    CHECK("find_max with n == 0 returns NULL", find_max(a, 0) == NULL, "expected NULL");
    m = find_max(b, 1);
    CHECK("find_max of one element points at it", m == b, "wrong pointer");

    CHECK("gap(a, a + 3) is 3", gap(a, a + 3) == 3, "got %td", gap(a, a + 3));
    CHECK("gap(a + 5, a + 2) is -3", gap(a + 5, a + 2) == -3, "got %td", gap(a + 5, a + 2));
    CHECK("gap(a, a) is 0", gap(a, a) == 0, "got %td", gap(a, a));
    CHECK("gap(a, a + 8) is 8", gap(a, a + 8) == 8, "got %td", gap(a, a + 8));
    return 0;
}
