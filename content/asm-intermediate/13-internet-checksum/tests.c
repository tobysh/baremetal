extern uint16_t inet_checksum(const void *data, size_t len);

static uint64_t seed = 1071;
static uint32_t rnd(void) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return (uint32_t)(seed >> 33); }
static uint16_t ref(const unsigned char *p, size_t n) {
    uint64_t s = 0;
    while (n >= 2) { s += p[0] | (p[1] << 8); p += 2; n -= 2; }
    if (n) s += p[0];
    while (s >> 16) s = (s & 0xFFFF) + (s >> 16);
    return (uint16_t)~s;
}

int main(void) {
    uint16_t c;
    c = inet_checksum("", 0);   CHECK("empty buffer gives 0xFFFF", c == 0xFFFF, "got 0x%04x", c);
    unsigned char z[4] = { 0, 0, 0, 0 };
    c = inet_checksum(z, 4);    CHECK("all zeros give 0xFFFF", c == 0xFFFF, "got 0x%04x", c);
    unsigned char o[1] = { 0x12 };
    c = inet_checksum(o, 1);    CHECK("one byte is the low half of a word", c == (uint16_t)~0x0012, "got 0x%04x", c);
    unsigned char w[2] = { 0x34, 0x12 };
    c = inet_checksum(w, 2);    CHECK("one word 0x1234 (little-endian)", c == (uint16_t)~0x1234, "got 0x%04x", c);
    unsigned char f[4] = { 0xFF, 0xFF, 0x02, 0x00 };
    c = inet_checksum(f, 4);    CHECK("carry is folded back in", c == (uint16_t)~0x0002, "got 0x%04x (0xFFFF + 2 = 0x10001, fold to 2)", c);

    /* The IPv4 header example from the Wikipedia article on the IPv4 header checksum: the correct value is 0xb861 in network order. */
    unsigned char ip[20] = { 0x45,0x00,0x00,0x73, 0x00,0x00,0x40,0x00, 0x40,0x11,0x00,0x00, 0xc0,0xa8,0x00,0x01, 0xc0,0xa8,0x00,0xc7 };
    c = inet_checksum(ip, 20);
    CHECK("real IPv4 header", c == 0x61b8, "got 0x%04x, expected 0x61b8 (0xb861 as stored bytes b8 61)", c);
    memcpy(ip + 10, &c, 2);
    c = inet_checksum(ip, 20);
    CHECK("header with its checksum inserted verifies to 0", c == 0, "got 0x%04x", c);

    unsigned char big[4100]; memset(big, 0xFF, sizeof big);
    c = inet_checksum(big, 4096); CHECK("4096 bytes of 0xFF (many carries)", c == ref(big, 4096), "got 0x%04x, expected 0x%04x", c, ref(big, 4096));
    int ok = 1; size_t bn = 0;
    for (size_t n = 0; n < 80 && ok; n++) {
        unsigned char b[100]; for (size_t i = 0; i < sizeof b; i++) b[i] = (unsigned char)rnd();
        if (inet_checksum(b, n) != ref(b, n)) { ok = 0; bn = n; }
    }
    CHECK("every length 0..79", ok, "wrong for length %zu", bn);
    ok = 1;
    for (int t = 0; t < 60 && ok; t++) {
        size_t n = rnd() % 3000, off = rnd() % 8; static unsigned char buf[3100];
        for (size_t i = 0; i < n + off; i++) buf[i] = (t & 1) ? 0xFF - (rnd() & 3) : (unsigned char)rnd();
        if (inet_checksum(buf + off, n) != ref(buf + off, n)) { ok = 0; bn = n; }
    }
    CHECK("random long unaligned buffers", ok, "wrong for length %zu", bn);
    return 0;
}
