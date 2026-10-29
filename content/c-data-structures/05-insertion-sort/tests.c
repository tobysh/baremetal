static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static int inversions(const int *a, int n) { int c = 0; for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) if (a[i] > a[j]) c++; return c; }
static int sorted(const int *a, int n) { for (int i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0; return 1; }

int main(void) {
    int a[] = {5, 2, 4, 6, 1, 3};
    int r = insertion_sort(a, 6);
    CHECK("sorts 5 2 4 6 1 3", sorted(a, 6) && a[0] == 1 && a[5] == 6, "got %d %d %d %d %d %d", a[0], a[1], a[2], a[3], a[4], a[5]);
    CHECK("that array needs 9 shifts", r == 9, "returned %d", r);

    int guard[3] = {7, 7, 7};
    CHECK("n = 0 does nothing and returns 0", insertion_sort(guard, 0) == 0 && guard[0] == 7, "failed");
    CHECK("n = 1 does nothing and returns 0", insertion_sort(guard, 1) == 0 && guard[0] == 7, "failed");

    int sortedin[] = {1, 2, 3, 4, 5, 6, 7, 8};
    CHECK("an already sorted array needs 0 shifts", insertion_sort(sortedin, 8) == 0 && sorted(sortedin, 8), "wrong result");

    int rev[10];
    for (int i = 0; i < 10; i++) rev[i] = 10 - i;
    r = insertion_sort(rev, 10);
    CHECK("a reversed array of 10 needs 45 shifts", r == 45 && sorted(rev, 10) && rev[0] == 1, "returned %d", r);

    int dup[] = {3, 3, 3, 1, 1, 2, 2, 3};
    int dupexp = inversions(dup, 8);
    r = insertion_sort(dup, 8);
    CHECK("duplicates: equal values are never shifted past each other", sorted(dup, 8) && r == dupexp, "returned %d, expected %d", r, dupexp);

    int neg[] = {0, -5, 7, -5, 2147483647, -2147483647 - 1, 0};
    int negexp = inversions(neg, 7);
    r = insertion_sort(neg, 7);
    CHECK("negative and extreme values", sorted(neg, 7) && neg[0] == -2147483647 - 1 && neg[6] == 2147483647 && r == negexp, "returned %d, expected %d", r, negexp);

    unsigned seed = 555u;
    int ok = 1, trial = 0;
    for (; trial < 60 && ok; trial++) {
        int n = (int)(rng(&seed) % 40u);
        int arr[40], copy[40];
        for (int i = 0; i < n; i++) arr[i] = copy[i] = (int)(rng(&seed) % 50u) - 25;
        int want = inversions(copy, n);
        int got = insertion_sort(arr, n);
        if (!sorted(arr, n) || got != want) ok = 0;
        long s1 = 0, s2 = 0;
        for (int i = 0; i < n; i++) { s1 += copy[i]; s2 += arr[i]; }
        if (s1 != s2) ok = 0;
    }
    CHECK("60 random arrays: sorted, same elements, correct shift count", ok, "failed on trial %d", trial);

    const char *words[] = {"pear", "apple", "fig", "banana", "apple", "cherry", "Zebra", "", "app"};
    const char *want[] = {"", "Zebra", "app", "apple", "apple", "banana", "cherry", "fig", "pear"};
    insertion_sort_strs(words, 9);
    ok = 1;
    for (int i = 0; i < 9; i++) if (strcmp(words[i], want[i]) != 0) ok = 0;
    CHECK("sorts strings by strcmp (uppercase and empty string first)", ok, "got %s %s %s %s ...", words[0], words[1], words[2], words[3]);

    const char *two[] = {"b", "a"};
    insertion_sort_strs(two, 2);
    CHECK("sorts two strings", strcmp(two[0], "a") == 0 && strcmp(two[1], "b") == 0, "got %s %s", two[0], two[1]);
    insertion_sort_strs(two, 0);
    insertion_sort_strs(two, 1);
    CHECK("string sort with n = 0 or 1 leaves the array alone", strcmp(two[0], "a") == 0 && strcmp(two[1], "b") == 0, "array changed");

    static const char lit[] = "abc";
    const char *same[] = {lit, "abc", lit};
    insertion_sort_strs(same, 3);
    CHECK("pointers move, characters are untouched", strcmp(lit, "abc") == 0 && strcmp(same[0], "abc") == 0, "string data changed");
    return 0;
}
