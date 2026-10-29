extern void *fill_bytes(void *dst, int c, size_t n);
extern void *move_bytes(void *dst, const void *src, size_t n);
extern size_t str_length(const char *s);

static uint64_t seed = 60221;
static uint32_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return (uint32_t)(seed >> 33); }
static int df_set(void) { unsigned long f; __asm__ volatile("pushfq; popq %0" : "=r"(f)); return (f >> 10) & 1; }

int main(void) {
    unsigned char b[64]; void *p;
    memset(b, 0xAA, sizeof b);
    p = fill_bytes(b + 8, 0x5C, 16);
    int ok = p == b + 8;
    for (int i = 0; i < 64; i++) { unsigned char e = (i >= 8 && i < 24) ? 0x5C : 0xAA; if (b[i] != e) ok = 0; }
    CHECK("fill_bytes fills exactly n bytes and returns dst", ok, "wrong contents or return value");
    fill_bytes(b, 0x1FF, 3);
    CHECK("only the low byte of c is used", b[0] == 0xFF && b[1] == 0xFF && b[2] == 0xFF, "got %02x %02x %02x", b[0], b[1], b[2]);
    memset(b, 1, sizeof b); fill_bytes(b, 9, 0);
    CHECK("fill_bytes with n = 0", b[0] == 1, "wrote with n = 0");
    ok = 1;
    for (int t = 0; t < 100 && ok; t++) {
        unsigned char x[100], y[100]; size_t off = rnd() % 20, n = rnd() % 60; int c = (int)rnd();
        for (int i = 0; i < 100; i++) x[i] = y[i] = (unsigned char)rnd();
        memset(y + off, c, n);
        if (fill_bytes(x + off, c, n) != x + off || memcmp(x, y, 100)) ok = 0;
    }
    CHECK("fill_bytes random", ok, "a random fill differed from memset");

    char s[64] = "0123456789abcdef";
    p = move_bytes(s + 20, s, 16);
    CHECK("move_bytes copies apart buffers", p == s + 20 && !memcmp(s + 20, "0123456789abcdef", 16), "got wrong data");
    char t[32] = "abcdefghij";
    move_bytes(t + 3, t, 7);
    CHECK("overlap, dst above src", !memcmp(t, "abcabcdefg", 10), "got \"%.10s\" (copy from the end)", t);
    char u[32] = "abcdefghij";
    move_bytes(u, u + 3, 7);
    CHECK("overlap, dst below src", !memcmp(u, "defghijhij", 10), "got \"%.10s\"", u);
    CHECK("direction flag is clear after move_bytes", !df_set(), "DF is set: execute cld before ret");
    ok = 1;
    for (int tt = 0; tt < 200 && ok; tt++) {
        unsigned char x[128], y[128]; size_t n = rnd() % 60, d = rnd() % 60, sr = rnd() % 60;
        for (int i = 0; i < 128; i++) x[i] = y[i] = (unsigned char)rnd();
        memmove(y + d, y + sr, n);
        void *rv = move_bytes(x + d, x + sr, n);
        if (rv != x + d || memcmp(x, y, 128)) ok = 0;
    }
    CHECK("move_bytes random overlapping moves", ok, "a random move differed from memmove");
    CHECK("direction flag still clear", !df_set(), "DF is set");

    size_t n;
    n = str_length("");          CHECK("empty string", n == 0, "got %zu", n);
    n = str_length("a");         CHECK("one char", n == 1, "got %zu", n);
    n = str_length("hello world"); CHECK("hello world", n == 11, "got %zu", n);
    ok = 1; size_t bn = 0;
    for (int i = 0; i < 100 && ok; i++) {
        char big[300]; size_t len = rnd() % 250;
        for (size_t k = 0; k < len; k++) big[k] = (char)(1 + rnd() % 255);
        big[len] = 0;
        if (str_length(big) != len) { ok = 0; bn = len; }
    }
    CHECK("str_length random strings", ok, "wrong for a string of length %zu", bn);
    return 0;
}
