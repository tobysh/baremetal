extern void isort(long *a, size_t n);
extern size_t count_inversions(const long *a, size_t n);

static uint64_t seed = 55555;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 29); }
static int cmp(const void *x, const void *y) { long a = *(const long *)x, b = *(const long *)y; return (a > b) - (a < b); }

int main(void) {
    long a[8] = { 5, 2, 9, 1, 5, 6, -3, 0 };
    long e[8] = { -3, 0, 1, 2, 5, 5, 6, 9 };
    isort(a, 8);  CHECK("small mixed array", !memcmp(a, e, sizeof a), "got %ld %ld %ld %ld %ld %ld %ld %ld", a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    long one[3] = { 7, 100, 100 };
    isort(one, 1); CHECK("n = 1 leaves the array alone", one[0] == 7 && one[1] == 100, "array changed");
    isort(one, 0); CHECK("n = 0 leaves the array alone", one[0] == 7, "array changed");
    long ex[4] = { 9223372036854775807L, -9223372036854775807L - 1, 0, -1 };
    isort(ex, 4);  CHECK("extreme values (signed compare)", ex[0] < ex[1] && ex[1] < ex[2] && ex[2] < ex[3], "got %ld %ld %ld %ld", ex[0], ex[1], ex[2], ex[3]);
    long dn[6] = { 6, 5, 4, 3, 2, 1 };
    isort(dn, 6);  CHECK("reverse-sorted input", dn[0] == 1 && dn[5] == 6, "got %ld ... %ld", dn[0], dn[5]);
    long guard[10]; for (int i = 0; i < 10; i++) guard[i] = 1000 + i;
    long mid[4] = { 3, 1, 2, 0 };
    long buf[10]; memcpy(buf, guard, sizeof buf); memcpy(buf + 3, mid, sizeof mid);
    isort(buf + 3, 4);
    CHECK("elements outside [0, n) are not touched", buf[2] == 1002 && buf[7] == 1007 && buf[3] == 0 && buf[6] == 3, "neighbours changed or sort wrong");
    int ok = 1; size_t bn = 0;
    for (int t = 0; t < 150 && ok; t++) {
        size_t n = rnd() % 60; long x[60], y[60];
        for (size_t i = 0; i < n; i++) x[i] = y[i] = (long)(rnd() >> (rnd() % 64)) * ((rnd() & 1) ? 1 : -1) % 50 - ((t & 1) ? 0 : 20);
        if (t % 5 == 0) for (size_t i = 0; i < n; i++) x[i] = y[i] = (long)rnd();
        qsort(y, n, sizeof(long), cmp);
        isort(x, n);
        if (memcmp(x, y, n * sizeof(long))) { ok = 0; bn = n; }
    }
    CHECK("isort random arrays", ok, "wrong for an array of %zu elements", bn);

    long s[5] = { 1, 2, 3, 4, 5 }, r5[5] = { 5, 4, 3, 2, 1 }, m[4] = { 2, 4, 1, 3 }, dup[3] = { 2, 2, 2 };
    size_t n;
    n = count_inversions(s, 5);   CHECK("sorted has 0 inversions", n == 0, "got %zu", n);
    n = count_inversions(r5, 5);  CHECK("reversed 5 has 10", n == 10, "got %zu", n);
    n = count_inversions(m, 4);   CHECK("{2,4,1,3} has 3", n == 3, "got %zu", n);
    n = count_inversions(dup, 3); CHECK("equal elements are not inversions", n == 0, "got %zu", n);
    n = count_inversions(s, 0);   CHECK("empty", n == 0, "got %zu", n);
    ok = 1;
    for (int t = 0; t < 100 && ok; t++) {
        size_t len = rnd() % 40; long x[40]; size_t exp = 0;
        for (size_t i = 0; i < len; i++) x[i] = (long)(rnd() % 30) - 15;
        if (t % 4 == 0) for (size_t i = 0; i < len; i++) x[i] = (long)rnd();
        for (size_t i = 0; i < len; i++) for (size_t j = i + 1; j < len; j++) if (x[i] > x[j]) exp++;
        if (count_inversions(x, len) != exp) { ok = 0; bn = len; }
    }
    CHECK("count_inversions random arrays", ok, "wrong for an array of %zu elements", bn);
    return 0;
}
