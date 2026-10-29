extern int clamp_byte(long x);
extern size_t count_in_range(const int8_t *p, size_t n, int lo, int hi);
extern unsigned nibble_at(uint64_t x, unsigned idx);

static uint64_t seed = 31337;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 30); }

int main(void) {
    int c;
    c = clamp_byte(100);   CHECK("clamp_byte inside", c == 100, "got %d", c);
    c = clamp_byte(0);     CHECK("clamp_byte 0", c == 0, "got %d", c);
    c = clamp_byte(255);   CHECK("clamp_byte 255", c == 255, "got %d", c);
    c = clamp_byte(256);   CHECK("clamp_byte 256 saturates", c == 255, "got %d", c);
    c = clamp_byte(-1);    CHECK("clamp_byte -1 saturates", c == 0, "got %d", c);
    c = clamp_byte(1L << 40);   CHECK("clamp_byte uses all 64 bits (high)", c == 255, "got %d", c);
    c = clamp_byte(-(1L << 40)); CHECK("clamp_byte uses all 64 bits (low)", c == 0, "got %d", c);
    int ok = 1; long bx = 0;
    for (int i = 0; i < 300 && ok; i++) {
        long x = (long)rnd() >> (rnd() % 62);
        int e = x < 0 ? 0 : x > 255 ? 255 : (int)x;
        if (clamp_byte(x) != e) { ok = 0; bx = x; }
    }
    CHECK("clamp_byte random", ok, "wrong for x = %ld", bx);

    int8_t a[8] = { -128, -5, 0, 5, 127, -1, 10, 3 };
    size_t n;
    n = count_in_range(a, 8, 0, 10);     CHECK("range 0..10", n == 4, "got %zu", n);
    n = count_in_range(a, 8, -128, 127); CHECK("full range", n == 8, "got %zu", n);
    n = count_in_range(a, 8, -10, -1);   CHECK("negative range (signed bytes)", n == 2, "got %zu", n);
    n = count_in_range(a, 8, 200, 300);  CHECK("out-of-byte range matches none", n == 0, "got %zu", n);
    n = count_in_range(a, 0, -128, 127); CHECK("empty array", n == 0, "got %zu", n);
    n = count_in_range(a, 8, 5, 4);      CHECK("lo > hi matches none", n == 0, "got %zu", n);
    ok = 1; int bl = 0, bh = 0;
    for (int t = 0; t < 100 && ok; t++) {
        int8_t buf[50]; size_t len = rnd() % 50; for (size_t i = 0; i < len; i++) buf[i] = (int8_t)rnd();
        int lo = (int)(rnd() % 300) - 150, hi = (int)(rnd() % 300) - 150;
        size_t e = 0; for (size_t i = 0; i < len; i++) if (buf[i] >= lo && buf[i] <= hi) e++;
        if (count_in_range(buf, len, lo, hi) != e) { ok = 0; bl = lo; bh = hi; }
    }
    CHECK("count_in_range random", ok, "wrong for lo=%d hi=%d", bl, bh);

    unsigned d;
    d = nibble_at(0x123456789abcdef0ULL, 0);   CHECK("nibble 0", d == 0x0, "got 0x%x", d);
    d = nibble_at(0x123456789abcdef0ULL, 15);  CHECK("nibble 15", d == 0x1, "got 0x%x", d);
    d = nibble_at(0x123456789abcdef0ULL, 8);   CHECK("nibble 8", d == 0x8, "got 0x%x", d);
    ok = 1; unsigned bi = 0;
    for (int i = 0; i < 300 && ok; i++) {
        uint64_t x = rnd(); unsigned idx = rnd() % 16;
        if (nibble_at(x, idx) != (unsigned)((x >> (4 * idx)) & 15)) { ok = 0; bi = idx; }
    }
    CHECK("nibble_at random", ok, "wrong for idx = %u", bi);
    return 0;
}
