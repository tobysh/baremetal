extern uint64_t get_bits(uint64_t x, unsigned lo, unsigned len);
extern uint32_t rotl32(uint32_t x, unsigned n);
extern uint64_t set_bits(uint64_t x, unsigned lo, unsigned len, uint64_t v);

static uint64_t seed = 4242;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 31); }
static uint64_t mask(unsigned len) { return len >= 64 ? ~0ULL : (1ULL << len) - 1; }

int main(void) {
    uint64_t r;
    r = get_bits(0xABCD, 4, 8);  CHECK("get_bits(0xABCD, 4, 8) = 0xBC", r == 0xBC, "got 0x%lx", r);
    r = get_bits(0xFF, 0, 0);    CHECK("get_bits with len 0", r == 0, "got 0x%lx", r);
    r = get_bits(~0ULL, 0, 63);  CHECK("get_bits of 63 ones", r == (~0ULL >> 1), "got 0x%lx", r);
    r = get_bits(1ULL << 63, 63, 1); CHECK("get_bits top bit", r == 1, "got 0x%lx", r);
    int ok = 1; uint64_t bx = 0; unsigned bl = 0, bn = 0;
    for (int i = 0; i < 400 && ok; i++) {
        uint64_t x = rnd(); unsigned lo = rnd() % 64, len = rnd() % 64;
        if (get_bits(x, lo, len) != ((x >> lo) & mask(len))) { ok = 0; bx = x; bl = lo; bn = len; }
    }
    CHECK("get_bits random", ok, "wrong for x=0x%lx lo=%u len=%u", bx, bl, bn);

    uint32_t q;
    q = rotl32(0x80000001u, 1);  CHECK("rotl32(0x80000001, 1) = 3", q == 3, "got 0x%x", q);
    q = rotl32(0x12345678u, 0);  CHECK("rotl32 by 0", q == 0x12345678u, "got 0x%x", q);
    q = rotl32(0x12345678u, 8);  CHECK("rotl32 by 8", q == 0x34567812u, "got 0x%x", q);
    ok = 1;
    for (int i = 0; i < 300 && ok; i++) {
        uint32_t x = (uint32_t)rnd(); unsigned n = rnd() % 32;
        uint32_t e = n ? (x << n) | (x >> (32 - n)) : x;
        if (rotl32(x, n) != e) { ok = 0; bx = x; bn = n; }
    }
    CHECK("rotl32 random", ok, "wrong for x=0x%lx n=%u", bx, bn);

    r = set_bits(0, 8, 4, 0xF);       CHECK("set_bits into zero", r == 0xF00, "got 0x%lx", r);
    r = set_bits(~0ULL, 8, 4, 0);     CHECK("set_bits clears a field", r == ~0xF00ULL, "got 0x%lx", r);
    r = set_bits(0, 4, 4, 0xFF);      CHECK("set_bits ignores high bits of v", r == 0xF0, "got 0x%lx", r);
    r = set_bits(0x1234, 60, 4, 0xA); CHECK("set_bits at the top", r == 0xA000000000001234ULL, "got 0x%lx", r);
    ok = 1;
    for (int i = 0; i < 400 && ok; i++) {
        uint64_t x = rnd(), v = rnd(); unsigned lo = rnd() % 64, len = rnd() % (65 - lo);
        uint64_t m = mask(len) << lo;
        uint64_t e = (x & ~m) | ((v << lo) & m);
        if (set_bits(x, lo, len, v) != e) { ok = 0; bx = x; bl = lo; bn = len; }
    }
    CHECK("set_bits random", ok, "wrong for x=0x%lx lo=%u len=%u", bx, bl, bn);
    return 0;
}
