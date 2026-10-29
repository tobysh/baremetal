static uint32_t rs = 777;
static uint8_t rnd(void) { rs = rs * 1664525u + 1013904223u; return (uint8_t)(rs >> 24); }

/* An independent reference, built bit by bit. */
static void ref_b64(const uint8_t *in, size_t n, char *out) {
    const char *T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    unsigned acc = 0; int bits = 0;
    for (size_t i = 0; i < n; i++) {
        acc = (acc << 8) | in[i]; bits += 8;
        while (bits >= 6) { out[o++] = T[(acc >> (bits - 6)) & 63]; bits -= 6; }
    }
    if (bits > 0) out[o++] = T[(acc << (6 - bits)) & 63];
    while (o % 4) out[o++] = '=';
    out[o] = '\0';
}

int main(void) {
    char out[128];
    size_t r;
    r = b64_encode((const uint8_t *)"", 0, out);
    CHECK("empty input gives an empty string", r == 0 && out[0] == '\0', "got \"%s\", length %zu", out, r);
    r = b64_encode((const uint8_t *)"Man", 3, out);
    CHECK("\"Man\" is TWFu (no padding)", strcmp(out, "TWFu") == 0 && r == 4, "got \"%s\"", out);
    b64_encode((const uint8_t *)"f", 1, out);
    CHECK("one leftover byte gives two = signs", strcmp(out, "Zg==") == 0, "got \"%s\"", out);
    b64_encode((const uint8_t *)"fo", 2, out);
    CHECK("two leftover bytes give one = sign", strcmp(out, "Zm8=") == 0, "got \"%s\"", out);
    b64_encode((const uint8_t *)"foobar", 6, out);
    CHECK("\"foobar\" is Zm9vYmFy", strcmp(out, "Zm9vYmFy") == 0, "got \"%s\"", out);
    uint8_t hi[] = {0xfb, 0xff, 0xfe};
    b64_encode(hi, 3, out);
    CHECK("the last two alphabet characters are + and /", strcmp(out, "+//+") == 0, "got \"%s\"", out);

    int ok = 1, bad = 0, lenok = 1;
    for (int n = 1; n <= 50 && ok; n++) {
        uint8_t buf[50];
        char want[100], got[100];
        for (int i = 0; i < n; i++) buf[i] = rnd();
        ref_b64(buf, (size_t)n, want);
        size_t len = b64_encode(buf, (size_t)n, got);
        if (strcmp(got, want) != 0) { ok = 0; bad = n; }
        if (len != strlen(want)) lenok = 0;
    }
    CHECK("matches a reference for random data of length 1 to 50", ok, "wrong for length %d", bad);
    CHECK("returns the length of the text", lenok, "the return value was not the string length");
    return 0;
}
