static int is_asc(const int *a, int n) {
    for (int i = 0; i + 1 < n; i++) if (a[i] > a[i + 1]) return 0;
    return 1;
}
static int same(const int *a, const int *b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != b[i]) return 0;
    return 1;
}
static unsigned int seed = 12345u;
static int next_rand(void) {
    seed = seed * 1103515245u + 12345u;
    return (int)((seed >> 16) % 2001u) - 1000;
}

int main(void) {
    int a[] = {5, 2, 9, 1, 5, 6};
    int want[] = {1, 2, 5, 5, 6, 9};
    insertion_sort(a, 6);
    CHECK("insertion_sort sorts a small array", same(a, want, 6), "got %d %d %d %d %d %d", a[0], a[1], a[2], a[3], a[4], a[5]);

    int neg[] = {3, -1, -7, 0, 3, -7};
    int wneg[] = {-7, -7, -1, 0, 3, 3};
    insertion_sort(neg, 6);
    CHECK("insertion_sort handles negatives and duplicates", same(neg, wneg, 6), "got %d %d %d %d %d %d", neg[0], neg[1], neg[2], neg[3], neg[4], neg[5]);

    int rev[] = {9, 8, 7, 6, 5, 4, 3, 2, 1};
    insertion_sort(rev, 9);
    CHECK("insertion_sort sorts a reversed array", is_asc(rev, 9) && rev[0] == 1 && rev[8] == 9, "not in order");

    int one[] = {42, 7};
    insertion_sort(one, 1);
    insertion_sort(one, 0);
    CHECK("insertion_sort with n = 0 or 1 changes nothing", one[0] == 42 && one[1] == 7, "got %d %d", one[0], one[1]);

    int pre[8] = {0};
    insertion_sort(pre + 1, 6);
    CHECK("insertion_sort stays inside its array (no writes before the start)", pre[0] == 0 && pre[7] == 0, "neighbouring items were changed");

    int ma[] = {1, 4, 9};
    int mb[] = {2, 3, 10};
    int mo[6];
    int r = merge_sorted(ma, 3, mb, 3, mo);
    int mwant[] = {1, 2, 3, 4, 9, 10};
    CHECK("merge_sorted merges two sorted arrays", r == 6 && same(mo, mwant, 6), "returned %d, got %d %d %d %d %d %d", r, mo[0], mo[1], mo[2], mo[3], mo[4], mo[5]);
    int mo2[3];
    r = merge_sorted(ma, 3, mb, 0, mo2);
    CHECK("merge_sorted with an empty right side copies the left", r == 3 && same(mo2, ma, 3), "returned %d", r);
    r = merge_sorted(ma, 0, mb, 3, mo2);
    CHECK("merge_sorted with an empty left side copies the right", r == 3 && same(mo2, mb, 3), "returned %d", r);
    r = merge_sorted(ma, 0, mb, 0, mo2);
    CHECK("merge_sorted of two empty arrays returns 0", r == 0, "returned %d", r);
    int sa[] = {1, 1, 2};
    int sb[] = {1, 2, 2};
    int so[6];
    r = merge_sorted(sa, 3, sb, 3, so);
    int swant[] = {1, 1, 1, 2, 2, 2};
    CHECK("merge_sorted copes with equal items", r == 6 && same(so, swant, 6), "got %d %d %d %d %d %d", so[0], so[1], so[2], so[3], so[4], so[5]);

    int m1[] = {5, 2, 9, 1, 5, 6};
    CHECK("merge_sort returns 0 on success", merge_sort(m1, 6) == 0, "expected 0");
    CHECK("merge_sort sorts a small array", same(m1, want, 6), "got %d %d %d %d %d %d", m1[0], m1[1], m1[2], m1[3], m1[4], m1[5]);

    int m2[] = {3, -1, -7, 0, 3, -7, 12};
    int wm2[] = {-7, -7, -1, 0, 3, 3, 12};
    merge_sort(m2, 7);
    CHECK("merge_sort handles an odd length with negatives and duplicates", same(m2, wm2, 7), "got %d %d %d %d %d %d %d", m2[0], m2[1], m2[2], m2[3], m2[4], m2[5], m2[6]);

    int m3[] = {42};
    CHECK("merge_sort of 0 or 1 items succeeds and changes nothing", merge_sort(m3, 1) == 0 && merge_sort(m3, 0) == 0 && m3[0] == 42, "bad result");

    int big[500];
    int big2[500];
    for (int i = 0; i < 500; i++) { big[i] = next_rand(); big2[i] = big[i]; }
    merge_sort(big, 500);
    insertion_sort(big2, 500);
    CHECK("merge_sort sorts 500 pseudo-random numbers", is_asc(big, 500), "not in ascending order");
    CHECK("merge_sort and insertion_sort agree (no items lost or invented)", same(big, big2, 500), "the two sorts gave different arrays");
    return 0;
}
