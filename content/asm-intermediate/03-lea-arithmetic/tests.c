#define LONG_MAX 9223372036854775807L
#define LONG_MIN (-9223372036854775807L - 1)
extern long times10(long x);
extern long nine_plus_five(long x);
extern long times85(long x);
extern long *elem_addr(long *base, long i);

int main(void) {
    long vals[] = { 0, 1, -1, 7, -13, 100, 123456789, -987654321, 1L << 40, -(1L << 50), LONG_MAX, LONG_MIN, 0x123456789abcdefL };
    int n = sizeof vals / sizeof vals[0];
    int ok = 1; long bad = 0;
    for (int i = 0; i < n && ok; i++) if (times10(vals[i]) != (long)((unsigned long)vals[i] * 10)) { ok = 0; bad = vals[i]; }
    CHECK("times10", ok, "wrong for x = %ld", bad);
    ok = 1;
    for (int i = 0; i < n && ok; i++) if (nine_plus_five(vals[i]) != (long)((unsigned long)vals[i] * 9 + 5)) { ok = 0; bad = vals[i]; }
    CHECK("nine_plus_five", ok, "wrong for x = %ld", bad);
    ok = 1;
    for (int i = 0; i < n && ok; i++) if (times85(vals[i]) != (long)((unsigned long)vals[i] * 85)) { ok = 0; bad = vals[i]; }
    CHECK("times85", ok, "wrong for x = %ld", bad);
    long arr[16];
    long *p = elem_addr(arr, 5);
    CHECK("elem_addr positive index", p == &arr[5], "got %p, expected %p", (void *)p, (void *)&arr[5]);
    p = elem_addr(&arr[8], -3);
    CHECK("elem_addr negative index", p == &arr[5], "got %p, expected %p", (void *)p, (void *)&arr[5]);
    p = elem_addr(arr, 0);
    CHECK("elem_addr index 0", p == arr, "got %p, expected %p", (void *)p, (void *)arr);
    return 0;
}
