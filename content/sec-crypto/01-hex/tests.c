static uint32_t rs = 12345;
static uint8_t rnd(void) { rs = rs * 1103515245u + 12345u; return (uint8_t)(rs >> 16); }

int main(void) {
    uint8_t a[] = {0x00, 0xff, 0x1a, 0xb0};
    char out[200];
    hex_encode(a, 4, out);
    CHECK("encodes 00 ff 1a b0 in lowercase", strcmp(out, "00ff1ab0") == 0, "got \"%s\"", out);
    strcpy(out, "zz");
    hex_encode(a, 0, out);
    CHECK("empty input gives an empty string", out[0] == '\0', "got \"%s\"", out);

    int ok = 1, len_seen = 0;
    for (int n = 1; n <= 40 && ok; n++) {
        uint8_t buf[40];
        char want[100], got[100];
        for (int i = 0; i < n; i++) { buf[i] = rnd(); sprintf(want + 2 * i, "%02x", buf[i]); }
        hex_encode(buf, (size_t)n, got);
        if (strcmp(got, want) != 0) { ok = 0; len_seen = n; }
    }
    CHECK("encodes random byte strings of length 1 to 40", ok, "wrong for length %d", len_seen);

    uint8_t d[8];
    memset(d, 0xaa, sizeof d);
    int n = hex_decode("DEADbeef01", d);
    CHECK("decodes mixed case and returns the byte count", n == 5 && d[0] == 0xde && d[1] == 0xad && d[2] == 0xbe && d[3] == 0xef && d[4] == 0x01, "returned %d", n);
    CHECK("decode does not write past the result", d[5] == 0xaa, "wrote beyond the end");
    CHECK("empty string decodes to 0 bytes", hex_decode("", d) == 0, "expected 0");
    CHECK("odd length is rejected", hex_decode("abc", d) == -1, "expected -1");
    CHECK("a non-hex character is rejected", hex_decode("0g", d) == -1, "expected -1");
    CHECK("a non-hex character in the second half is rejected", hex_decode("00112z", d) == -1, "expected -1");

    ok = 1;
    for (int len = 1; len <= 40 && ok; len++) {
        uint8_t buf[40], back[40];
        char h[100];
        for (int i = 0; i < len; i++) buf[i] = rnd();
        hex_encode(buf, (size_t)len, h);
        int m = hex_decode(h, back);
        if (m != len || memcmp(buf, back, (size_t)len) != 0) { ok = 0; len_seen = len; }
    }
    CHECK("decode(encode(x)) gives x back", ok, "round trip failed for length %d", len_seen);
    return 0;
}
