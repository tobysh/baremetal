static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

// Slow reference: no bit tricks.
static int ref_pow2(size_t a) { size_t p = 1; for (int i = 0; i < 64; i++, p <<= 1) { if (p == a) return 1; if (p == 0) break; } return 0; }
static size_t ref_up(size_t n, size_t a) {
    if (!ref_pow2(a)) return SIZE_MAX;
    size_t r = n % a;
    if (r == 0) return n;
    size_t add = a - r;
    if (n > SIZE_MAX - add) return SIZE_MAX;
    return n + add;
}

struct A { char c; int i; char d; double x; short s; };
struct B { char a; char b; char c; };
struct C { short s; char c; long long q; char t; };
struct D { double d; char c; };
struct E { char c; void *p; int i; char t[3]; };

#define MEMBER(T, f) { sizeof(((T *)0)->f), _Alignof(__typeof__(((T *)0)->f)), offsetof(T, f) }
typedef struct { size_t size, align, off; } Mem;

static int check_struct(const Mem *m, int n, size_t want_size, size_t want_align, char *why, size_t whylen) {
    size_t sz[8], al[8], off[8], expect_total;
    (void)want_align;
    for (int i = 0; i < n; i++) { sz[i] = m[i].size; al[i] = m[i].align; off[i] = 12345; }
    expect_total = want_size;
    size_t t = layout(sz, al, n, off);
    for (int i = 0; i < n; i++) {
        if (off[i] != m[i].off) { snprintf(why, whylen, "member %d at offset %zu, compiler says %zu", i, off[i], m[i].off); return 0; }
    }
    if (t != expect_total) { snprintf(why, whylen, "total %zu, compiler says %zu", t, expect_total); return 0; }
    return 1;
}

int main(void) {
    CHECK("is_pow2 accepts powers of two", is_pow2(1) && is_pow2(2) && is_pow2(64) && is_pow2((size_t)1 << 63),
          "is_pow2(1)=%d is_pow2(2)=%d is_pow2(64)=%d", is_pow2(1), is_pow2(2), is_pow2(64));
    CHECK("is_pow2 rejects zero and non-powers", !is_pow2(0) && !is_pow2(3) && !is_pow2(12) && !is_pow2(SIZE_MAX) && !is_pow2(((size_t)1 << 63) + 1),
          "is_pow2(0)=%d is_pow2(3)=%d is_pow2(12)=%d", is_pow2(0), is_pow2(3), is_pow2(12));

    CHECK("align_up of the lesson example", align_up(13, 8) == 16 && align_up(16, 8) == 16 && align_up(0, 16) == 0 && align_up(1, 1) == 1,
          "align_up(13,8)=%zu align_up(16,8)=%zu align_up(0,16)=%zu", align_up(13, 8), align_up(16, 8), align_up(0, 16));

    unsigned s = 20241029u;
    size_t as[] = { 1, 2, 4, 8, 16, 64, 4096 };
    size_t bad_up = 0, bad_down = 0, bad_pad = 0, n_bad = 0, a_bad = 0, shown_n = 0, shown_a = 0;
    for (int i = 0; i < 4000; i++) {
        size_t a = as[rng(&s) % 7];
        size_t n = ((size_t)rng(&s) << 16 | rng(&s)) % 100000;
        if (align_up(n, a) != ref_up(n, a)) { if (!bad_up++) { shown_n = n; shown_a = a; } }
        size_t down = n - n % a;
        if (align_down(n, a) != down) { if (!bad_down++) { n_bad = n; a_bad = a; } }
        uintptr_t addr = (uintptr_t)n * 7 + 3;
        size_t pad = ref_up(addr, a) - addr;
        if (padding_for(addr, a) != pad) bad_pad++;
    }
    CHECK("align_up matches the loop reference on 4000 random inputs", bad_up == 0, "%zu mismatches, first: align_up(%zu, %zu) = %zu, expected %zu", bad_up, shown_n, shown_a, align_up(shown_n, shown_a), ref_up(shown_n, shown_a));
    CHECK("align_down matches the reference", bad_down == 0, "%zu mismatches, first: align_down(%zu, %zu) = %zu", bad_down, n_bad, a_bad, align_down(n_bad, a_bad));
    CHECK("padding_for matches the reference", bad_pad == 0, "%zu mismatches", bad_pad);

    CHECK("align_up rejects a bad alignment", align_up(10, 0) == SIZE_MAX && align_up(10, 12) == SIZE_MAX && align_up(0, 3) == SIZE_MAX,
          "align_up(10,0)=%zu align_up(10,12)=%zu", align_up(10, 0), align_up(10, 12));
    CHECK("align_down and padding_for reject a bad alignment", align_down(10, 0) == SIZE_MAX && align_down(10, 6) == SIZE_MAX && padding_for(10, 6) == SIZE_MAX && padding_for(10, 0) == SIZE_MAX,
          "align_down(10,6)=%zu padding_for(10,6)=%zu", align_down(10, 6), padding_for(10, 6));

    size_t top = SIZE_MAX;
    CHECK("align_up detects overflow near SIZE_MAX", align_up(top, 8) == SIZE_MAX && align_up(top - 6, 8) == SIZE_MAX && align_up(top - 7, 8) == top - 7,
          "align_up(SIZE_MAX,8)=%zu align_up(SIZE_MAX-6,8)=%zu align_up(SIZE_MAX-7,8)=%zu (SIZE_MAX-7 is already a multiple of 8)", align_up(top, 8), align_up(top - 6, 8), align_up(top - 7, 8));
    CHECK("align_up(SIZE_MAX, 1) does not overflow", align_up(top, 1) == top, "got %zu", align_up(top, 1));
    CHECK("align_down works at the top of the range", align_down(top, 4096) == top - 4095 && align_down(top, 1) == top, "align_down(SIZE_MAX,4096)=%zu", align_down(top, 4096));
    CHECK("padding_for near the top of the address space", padding_for(top - 2, 8) == 3 && padding_for(top - 7, 8) == 0, "padding_for(SIZE_MAX-2,8)=%zu padding_for(SIZE_MAX-7,8)=%zu (expected 3 and 0)", padding_for(top - 2, 8), padding_for(top - 7, 8));

    char why[96] = "";
    Mem ma[] = { MEMBER(struct A, c), MEMBER(struct A, i), MEMBER(struct A, d), MEMBER(struct A, x), MEMBER(struct A, s) };
    CHECK("layout matches the compiler: char, int, char, double, short", check_struct(ma, 5, sizeof(struct A), _Alignof(struct A), why, sizeof why), "%s", why);
    Mem mb[] = { MEMBER(struct B, a), MEMBER(struct B, b), MEMBER(struct B, c) };
    CHECK("layout matches the compiler: three chars need no padding", check_struct(mb, 3, sizeof(struct B), 1, why, sizeof why), "%s", why);
    Mem mc[] = { MEMBER(struct C, s), MEMBER(struct C, c), MEMBER(struct C, q), MEMBER(struct C, t) };
    CHECK("layout matches the compiler: short, char, long long, char", check_struct(mc, 4, sizeof(struct C), _Alignof(struct C), why, sizeof why), "%s", why);
    Mem md[] = { MEMBER(struct D, d), MEMBER(struct D, c) };
    CHECK("layout pads the tail: double then char", check_struct(md, 2, sizeof(struct D), 8, why, sizeof why), "%s", why);
    Mem me[] = { MEMBER(struct E, c), MEMBER(struct E, p), MEMBER(struct E, i), MEMBER(struct E, t) };
    CHECK("layout matches the compiler: char, pointer, int, char[3]", check_struct(me, 4, sizeof(struct E), 8, why, sizeof why), "%s", why);

    size_t o1[1] = { 9 };
    size_t one_s[1] = { 5 }, one_a[1] = { 1 };
    CHECK("layout of one byte-aligned member", layout(one_s, one_a, 1, o1) == 5 && o1[0] == 0, "total %zu offset %zu", layout(one_s, one_a, 1, o1), o1[0]);
    CHECK("layout of nothing is 0", layout(one_s, one_a, 0, o1) == 0 && layout(one_s, one_a, -3, o1) == 0, "returned non-zero for n <= 0");
    return 0;
}
