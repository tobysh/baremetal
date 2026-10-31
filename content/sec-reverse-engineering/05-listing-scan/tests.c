/* Independent reference: written differently from the expected shape. */
static int ref_scan(const int *a, int n) {
    int idx = -1;
    for (int i = n - 1; i >= 0; i--) {
        if (a[i] <= 0) continue;
        if (idx < 0 || a[i] <= a[idx]) idx = i;
    }
    return idx;
}
static unsigned rs(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    int e[1] = {0};
    CHECK("empty array returns -1", scan(e, 0) == -1 && scan(e, -3) == -1, "got %d", scan(e, 0));
    int neg[] = {-4, 0, -1, 0};
    CHECK("no positive element returns -1", scan(neg, 4) == -1, "got %d", scan(neg, 4));
    int one[] = {0, 7};
    CHECK("single positive element", scan(one, 2) == 1, "got %d", scan(one, 2));
    int tie[] = {9, 4, 6, 4, 4, 10};
    CHECK("first of equal minimums wins", scan(tie, 6) == 1, "got %d, expected 1", scan(tie, 6));
    int zeroes[] = {0, 0, 3, 0, 2, -9};
    CHECK("zero and negative values are skipped", scan(zeroes, 6) == 4, "got %d, expected 4", scan(zeroes, 6));
    int big[] = {INT_MAX, INT_MAX - 1, INT_MAX};
    CHECK("large values", scan(big, 3) == 1, "got %d", scan(big, 3));
    int n = 3; int pre[] = {5, 1, 0, 9};
    CHECK("elements at and past n are never read", scan(pre, n) == 1 && scan(pre, 1) == 0, "got %d", scan(pre, n));
    unsigned s = 7; int ok = 1, bad = -1;
    for (int t = 0; t < 400 && ok; t++) {
        int arr[24]; int len = (int)(rs(&s) % 25);
        for (int i = 0; i < len; i++) arr[i] = (int)(rs(&s) % 21) - 8;
        if (scan(arr, len) != ref_scan(arr, len)) { ok = 0; bad = t; }
    }
    CHECK("400 random arrays agree with the reference", ok, "first mismatch in array %d", bad);
    return 0;
}
