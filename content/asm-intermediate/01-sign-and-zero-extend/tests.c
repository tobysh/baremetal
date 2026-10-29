extern long sum_i8(const int8_t *p, size_t n);
extern unsigned long sum_u8(const uint8_t *p, size_t n);
extern long sum_i32(const int32_t *p, size_t n);

static uint64_t seed = 12345;
static uint32_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return (uint32_t)(seed >> 33); }

int main(void) {
    int8_t a[4] = { -1, 2, -3, 4 };
    uint8_t u[2] = { 200, 100 };
    long r; unsigned long ur;
    r = sum_i8(a, 4);   CHECK("sum_i8 small", r == 2, "got %ld", r);
    r = sum_i8(a, 0);   CHECK("sum_i8 empty", r == 0, "got %ld", r);
    ur = sum_u8(u, 2);  CHECK("sum_u8 {200,100} = 300", ur == 300, "got %lu (zero-extend with movzx)", ur);
    int8_t *neg = (int8_t *)u;
    r = sum_i8(neg, 2); CHECK("sum_i8 treats 200 as -56", r == 44, "got %ld (sign-extend with movsx)", r);

    int ok = 1; long bad_n = 0;
    for (int t = 0; t < 40 && ok; t++) {
        size_t n = rnd() % 70;
        int8_t buf[80]; long e = 0;
        for (size_t i = 0; i < n; i++) { buf[i] = (int8_t)rnd(); e += buf[i]; }
        if (sum_i8(buf, n) != e) { ok = 0; bad_n = (long)n; }
    }
    CHECK("sum_i8 random arrays", ok, "wrong for an array of %ld bytes", bad_n);
    ok = 1;
    for (int t = 0; t < 40 && ok; t++) {
        size_t n = rnd() % 70;
        uint8_t buf[80]; unsigned long e = 0;
        for (size_t i = 0; i < n; i++) { buf[i] = (uint8_t)rnd(); e += buf[i]; }
        if (sum_u8(buf, n) != e) { ok = 0; bad_n = (long)n; }
    }
    CHECK("sum_u8 random arrays", ok, "wrong for an array of %ld bytes", bad_n);
    int32_t w[3] = { 2000000000, 2000000000, -5 };
    r = sum_i32(w, 3);  CHECK("sum_i32 does not wrap at 32 bits", r == 3999999995L, "got %ld", r);
    ok = 1;
    for (int t = 0; t < 40 && ok; t++) {
        size_t n = rnd() % 50;
        int32_t buf[60]; long e = 0;
        for (size_t i = 0; i < n; i++) { buf[i] = (int32_t)rnd() * (rnd() & 1 ? 1 : -1) ; e += buf[i]; }
        if (sum_i32(buf, n) != e) { ok = 0; bad_n = (long)n; }
    }
    CHECK("sum_i32 random arrays", ok, "wrong for an array of %ld ints", bad_n);
    return 0;
}
