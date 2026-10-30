int main(void) {
    char *r = repeat_str("ab", 3);
    CHECK("repeat_str(\"ab\", 3) is \"ababab\"", r != NULL && strcmp(r, "ababab") == 0, "got \"%s\"", r ? r : "(null)");
    free(r);

    r = repeat_str("x", 1);
    CHECK("repeat_str(\"x\", 1) is \"x\"", r != NULL && strcmp(r, "x") == 0, "got \"%s\"", r ? r : "(null)");
    free(r);

    r = repeat_str("abc", 0);
    CHECK("repeat_str with times 0 is an empty string, not NULL", r != NULL && r[0] == '\0', "got %s", r ? "non-empty" : "(null)");
    free(r);

    r = repeat_str("", 5);
    CHECK("repeat_str of the empty string is empty", r != NULL && r[0] == '\0', "got %s", r ? "non-empty" : "(null)");
    free(r);

    r = repeat_str("Hi! ", 50);
    int ok = r != NULL && strlen(r) == 200;
    for (int i = 0; ok && i < 50; i++) if (strncmp(r + 4 * i, "Hi! ", 4) != 0) ok = 0;
    CHECK("repeat_str(\"Hi! \", 50) has 200 characters of repeated text", ok, "bad length or content");
    free(r);

    int a[] = {1, 2, 3, 4, 6, 7, -8, 0, 11};
    size_t count = 99;
    int *e = evens(a, 9, &count);
    int expected[] = {2, 4, 6, -8, 0};
    int eok = e != NULL && count == 5;
    for (int i = 0; eok && i < 5; i++) if (e[i] != expected[i]) eok = 0;
    CHECK("evens keeps 2, 4, 6, -8, 0 in order and reports 5", eok, "count = %zu", count);
    free(e);

    int odd[] = {1, 3, 5};
    count = 99;
    e = evens(odd, 3, &count);
    CHECK("evens of all-odd input reports 0 and returns a valid block", e != NULL && count == 0, "count = %zu, ptr = %p", count, (void *)e);
    free(e);

    count = 99;
    e = evens(odd, 0, &count);
    CHECK("evens of an empty array reports 0", e != NULL && count == 0, "count = %zu", count);
    free(e);

    int big[1000];
    for (int i = 0; i < 1000; i++) big[i] = i;
    count = 0;
    e = evens(big, 1000, &count);
    eok = e != NULL && count == 500;
    for (size_t i = 0; eok && i < 500; i++) if (e[i] != (int)(2 * i)) eok = 0;
    CHECK("evens of 0..999 returns 500 values 0, 2, 4, ...", eok, "count = %zu", count);
    free(e);

    int lo = 0, hi = 0;
    min_max(a, 9, &lo, &hi);
    CHECK("min_max finds -8 and 11", lo == -8 && hi == 11, "min = %d, max = %d", lo, hi);

    int one[] = {42};
    min_max(one, 1, &lo, &hi);
    CHECK("min_max of one element gives it for both", lo == 42 && hi == 42, "min = %d, max = %d", lo, hi);

    int neg[] = {-5, -2, -9};
    min_max(neg, 3, &lo, &hi);
    CHECK("min_max handles all-negative values", lo == -9 && hi == -2, "min = %d, max = %d", lo, hi);

    int mid[] = {3, 1, 4, 1, 5};
    min_max(mid + 1, 3, &lo, &hi);
    CHECK("min_max works on a sub-array starting mid-way", lo == 1 && hi == 4, "min = %d, max = %d", lo, hi);
    return 0;
}
