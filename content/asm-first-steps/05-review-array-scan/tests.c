extern long sum_clamped(const long *a, size_t n, long lo, long hi);
extern size_t count_negative(const long *a, size_t n);

int main(void) {
    long a[] = {-10, -3, 0, 4, 9, 50};
    long poison[1] = {999};

    CHECK("sum_clamped keeps values already inside the range", sum_clamped(a + 2, 2, -5, 5) == 4, "got %ld", sum_clamped(a + 2, 2, -5, 5));
    CHECK("sum_clamped clamps both ends (-5..5)", sum_clamped(a, 6, -5, 5) == -5 + -3 + 0 + 4 + 5 + 5, "got %ld", sum_clamped(a, 6, -5, 5));
    CHECK("sum_clamped(n = 0) is 0 without reading", sum_clamped(poison, 0, 0, 10) == 0, "got %ld (n = 0 must not read the array)", sum_clamped(poison, 0, 0, 10));
    CHECK("sum_clamped uses only the first n elements", sum_clamped(a, 3, -100, 100) == -13, "got %ld", sum_clamped(a, 3, -100, 100));
    CHECK("sum_clamped with lo == hi gives n * lo", sum_clamped(a, 6, 7, 7) == 42, "got %ld", sum_clamped(a, 6, 7, 7));
    CHECK("sum_clamped works with an all-negative range", sum_clamped(a, 6, -20, -1) == -10 + -3 + -1 + -1 + -1 + -1, "got %ld", sum_clamped(a, 6, -20, -1));
    long big[] = {4000000000L, -4000000000L, 5000000000L};
    CHECK("sum_clamped handles 64-bit bounds", sum_clamped(big, 3, -3000000000L, 4500000000L) == 4000000000L - 3000000000L + 4500000000L,
          "got %ld", sum_clamped(big, 3, -3000000000L, 4500000000L));

    long many[500];
    long expect = 0;
    for (int i = 0; i < 500; i++) {
        many[i] = (long)i * 7 - 1000;
        long c = many[i] < -200 ? -200 : (many[i] > 300 ? 300 : many[i]);
        expect += c;
    }
    CHECK("sum_clamped over 500 elements", sum_clamped(many, 500, -200, 300) == expect, "got %ld, expected %ld", sum_clamped(many, 500, -200, 300), expect);

    long keep[3] = {-9, 0, 9};
    sum_clamped(keep, 3, -1, 1);
    CHECK("sum_clamped does not modify the array", keep[0] == -9 && keep[1] == 0 && keep[2] == 9, "array is now {%ld, %ld, %ld}", keep[0], keep[1], keep[2]);

    long neg[] = {-1, 5, -7, 0, 3, -2};
    CHECK("count_negative counts 3 in a mixed array", count_negative(neg, 6) == 3, "got %zu", count_negative(neg, 6));
    CHECK("count_negative(n = 0) is 0 without reading", count_negative(poison, 0) == 0, "got %zu", count_negative(poison, 0));
    CHECK("count_negative does not count zero", count_negative(neg + 3, 1) == 0, "got %zu", count_negative(neg + 3, 1));
    CHECK("count_negative uses only the first n elements", count_negative(neg, 3) == 2, "got %zu", count_negative(neg, 3));
    long minl[] = {(long)0x8000000000000000UL, 1};
    CHECK("count_negative treats the most negative long as negative", count_negative(minl, 2) == 1, "got %zu", count_negative(minl, 2));
    long pos[] = {1, 2, 3};
    CHECK("count_negative of all positives is 0", count_negative(pos, 3) == 0, "got %zu", count_negative(pos, 3));
    long allneg[] = {-1, -2, -3, -4};
    CHECK("count_negative of all negatives is n", count_negative(allneg, 4) == 4, "got %zu", count_negative(allneg, 4));
    return 0;
}
