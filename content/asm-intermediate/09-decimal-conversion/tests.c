extern size_t u64_to_dec(uint64_t v, char *buf);
extern long parse_dec(const char *s);

static uint64_t seed = 1009;
static uint64_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return seed ^ (seed >> 29); }

static int one(uint64_t v) {
    char buf[40], ref[40];
    memset(buf, 'Z', sizeof buf);
    size_t n = u64_to_dec(v, buf);
    int len = snprintf(ref, sizeof ref, "%lu", v);
    if (n != (size_t)len || memcmp(buf, ref, len + 1)) return 0;
    for (int i = len + 1; i < 40; i++) if (buf[i] != 'Z') return 0;
    return 1;
}

int main(void) {
    char buf[40]; size_t n;
    n = u64_to_dec(0, buf);       CHECK("0 becomes \"0\"", n == 1 && !strcmp(buf, "0"), "returned %zu, buffer \"%s\"", n, buf);
    n = u64_to_dec(7, buf);       CHECK("one digit", n == 1 && !strcmp(buf, "7"), "returned %zu, buffer \"%s\"", n, buf);
    n = u64_to_dec(1203, buf);    CHECK("1203", n == 4 && !strcmp(buf, "1203"), "returned %zu, buffer \"%s\"", n, buf);
    n = u64_to_dec(1000000, buf); CHECK("trailing zeros are kept", n == 7 && !strcmp(buf, "1000000"), "returned %zu, buffer \"%s\"", n, buf);
    n = u64_to_dec(~0UL, buf);    CHECK("2^64 - 1", n == 20 && !strcmp(buf, "18446744073709551615"), "returned %zu, buffer \"%s\"", n, buf);
    int ok = 1; uint64_t bv = 0;
    for (int i = 0; i < 300 && ok; i++) { uint64_t v = rnd() >> (rnd() % 64); if (!one(v)) { ok = 0; bv = v; } }
    CHECK("u64_to_dec random values, no writes past the NUL", ok, "wrong for %lu", bv);

    long r;
    r = parse_dec("0");          CHECK("parse 0", r == 0, "got %ld", r);
    r = parse_dec("12345");      CHECK("parse 12345", r == 12345, "got %ld", r);
    r = parse_dec("-42");        CHECK("parse -42", r == -42, "got %ld", r);
    r = parse_dec("+17");        CHECK("parse +17", r == 17, "got %ld", r);
    r = parse_dec("99abc");      CHECK("stops at a non-digit", r == 99, "got %ld", r);
    r = parse_dec("");           CHECK("empty string", r == 0, "got %ld", r);
    r = parse_dec("-");          CHECK("sign only", r == 0, "got %ld", r);
    r = parse_dec("x5");         CHECK("no leading digit", r == 0, "got %ld", r);
    r = parse_dec("/9");         CHECK("'/' (just below '0') is not a digit", r == 0, "got %ld", r);
    r = parse_dec("9223372036854775807");  CHECK("LONG_MAX", r == 9223372036854775807L, "got %ld", r);
    r = parse_dec("-9223372036854775807"); CHECK("-LONG_MAX", r == -9223372036854775807L, "got %ld", r);
    ok = 1; long bl = 0;
    for (int i = 0; i < 300 && ok; i++) {
        long v = (long)rnd() >> (rnd() % 63); char s[48];
        snprintf(s, sizeof s, "%ld%s", v, (i & 1) ? "xyz" : "");
        if (parse_dec(s) != v) { ok = 0; bl = v; }
    }
    CHECK("parse_dec random values", ok, "wrong for %ld", bl);
    return 0;
}
