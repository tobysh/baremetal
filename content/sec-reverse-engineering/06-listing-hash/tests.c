/* Independent reference: 64-bit arithmetic masked to 32 bits, rotate by arithmetic. */
static uint32_t ref_mix(const unsigned char *p, size_t n) {
    uint64_t h = 524114809ull;
    for (size_t i = 0; i < n; i++) {
        h = ((h * 32ull) % 4294967296ull) + (h / 134217728ull);
        h ^= p[i];
        h = (h * 625341585ull) % 4294967296ull;
    }
    return (uint32_t)(h ^ (h >> 15));
}
static unsigned rs(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    CHECK("empty buffer", mix((const unsigned char *)"", 0) == ref_mix((const unsigned char *)"", 0), "got %u, expected %u", (unsigned)mix((const unsigned char *)"", 0), (unsigned)ref_mix((const unsigned char *)"", 0));
    unsigned char one[1] = {0};
    CHECK("one zero byte", mix(one, 1) == ref_mix(one, 1), "got %u, expected %u", (unsigned)mix(one, 1), (unsigned)ref_mix(one, 1));
    unsigned char hi[3] = {0x80, 0xFF, 0xC3};
    CHECK("bytes of 128 and above are unsigned", mix(hi, 3) == ref_mix(hi, 3), "got %u, expected %u", (unsigned)mix(hi, 3), (unsigned)ref_mix(hi, 3));
    CHECK("text", mix((const unsigned char *)"abcdef", 6) == ref_mix((const unsigned char *)"abcdef", 6), "got %u", (unsigned)mix((const unsigned char *)"abcdef", 6));
    unsigned char buf[300]; unsigned s = 99; int ok = 1; int badlen = -1;
    for (int t = 0; t < 300 && ok; t++) {
        size_t len = rs(&s) % 300;
        for (size_t i = 0; i < len; i++) buf[i] = (unsigned char)rs(&s);
        if (mix(buf, len) != ref_mix(buf, len)) { ok = 0; badlen = (int)len; }
    }
    CHECK("300 random buffers agree with the reference", ok, "first mismatch at length %d", badlen);
    return 0;
}
