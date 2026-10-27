int main(void) {
    int a[] = {4, 8, 15, 16};
    int one[] = {7};
    CHECK("sum_all adds every item, including the first", sum_all(a, 4) == 43, "got %d, expected 43", sum_all(a, 4));
    CHECK("sum_all of a single item", sum_all(one, 1) == 7, "got %d, expected 7", sum_all(one, 1));
    CHECK("sum_all of nothing is 0", sum_all(a, 0) == 0, "got %d", sum_all(a, 0));

    CHECK("count_char counts matches in the middle", count_char("banana", 'n') == 2, "got %d, expected 2", count_char("banana", 'n'));
    CHECK("count_char sees the last character", count_char("banana", 'a') == 3, "got %d, expected 3", count_char("banana", 'a'));
    CHECK("count_char on a one-character string", count_char("x", 'x') == 1, "got %d, expected 1", count_char("x", 'x'));
    CHECK("count_char with no matches", count_char("abc", 'z') == 0 && count_char("", 'a') == 0, "expected 0");

    int neg[] = {-8, -3, -20};
    int mixed[] = {3, 9, 2};
    CHECK("max_of works when every item is negative", max_of(neg, 3) == -3, "got %d, expected -3", max_of(neg, 3));
    CHECK("max_of finds a normal maximum", max_of(mixed, 3) == 9, "got %d, expected 9", max_of(mixed, 3));
    CHECK("max_of a single negative item", max_of(&neg[2], 1) == -20, "got %d, expected -20", max_of(&neg[2], 1));

    int s1[] = {1, 2, 2, 5};
    int s2[] = {1, 2, 5, 3};
    int s3[] = {1, 9, 3};
    int s4[] = {2, 1};
    CHECK("is_sorted accepts equal neighbours", is_sorted(s1, 4) == 1, "expected 1");
    CHECK("is_sorted catches a problem in the last pair", is_sorted(s2, 4) == 0, "expected 0 for {1, 2, 5, 3}");
    CHECK("is_sorted catches a problem in the middle", is_sorted(s3, 3) == 0, "expected 0 for {1, 9, 3}");
    CHECK("is_sorted catches an unsorted pair", is_sorted(s4, 2) == 0, "expected 0 for {2, 1}");
    CHECK("is_sorted of 0 or 1 items is true", is_sorted(s1, 0) == 1 && is_sorted(one, 1) == 1, "expected 1");

    int r1[] = {1, 2, 3, 4, 5};
    int r2[] = {1, 2, 3, 4};
    reverse(r1, 5);
    reverse(r2, 4);
    CHECK("reverse flips an odd-length array", r1[0] == 5 && r1[1] == 4 && r1[2] == 3 && r1[3] == 2 && r1[4] == 1, "got %d %d %d %d %d", r1[0], r1[1], r1[2], r1[3], r1[4]);
    CHECK("reverse flips an even-length array", r2[0] == 4 && r2[1] == 3 && r2[2] == 2 && r2[3] == 1, "got %d %d %d %d", r2[0], r2[1], r2[2], r2[3]);
    int r3[] = {9};
    reverse(r3, 1);
    reverse(r3, 0);
    CHECK("reverse leaves 0 or 1 items alone", r3[0] == 9, "got %d", r3[0]);

    int av[] = {2, 3};
    int av2[] = {1, 2, 2};
    CHECK("average keeps the fraction", average(av, 2) > 2.499 && average(av, 2) < 2.501, "got %f, expected 2.5", average(av, 2));
    CHECK("average of {1, 2, 2} is 1.666...", average(av2, 3) > 1.66 && average(av2, 3) < 1.67, "got %f", average(av2, 3));
    CHECK("average of nothing is 0.0", average(av, 0) == 0.0, "got %f", average(av, 0));
    return 0;
}
