static uint32_t st = 5;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static void w16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void w32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (24 - 8 * i)); }

/* Builds `n` bytes: a header with the given fields, random bytes after byte 20. */
static void ip(uint8_t *b, size_t n, unsigned ver, unsigned ihl, unsigned total, unsigned ff, unsigned proto, uint32_t src, uint32_t dst)
{
    for (size_t i = 0; i < n; i++)
        b[i] = (uint8_t)rnd();
    b[0] = (uint8_t)(ver << 4 | ihl);
    b[1] = 0;
    w16(b + 2, total);
    w16(b + 6, ff);
    b[9] = (uint8_t)proto;
    w32(b + 12, src);
    w32(b + 16, dst);
}

int main(void)
{
    uint8_t b[128];
    IPv4 v;
    IPv4 sentinel;
    memset(&sentinel, 0x5a, sizeof sentinel);

    for (unsigned ihl = 5; ihl <= 15; ihl += 5) {
        size_t h = ihl * 4;
        size_t total = h + (rnd() % 8);
        uint32_t s = 0xc0000200u | (rnd() & 0xff), d = 0xcb007100u | (rnd() & 0xff);
        ip(b, total, 4, ihl, (unsigned)total, 0, 6, s, d);
        memset(&v, 0, sizeof v);
        int rc = parse_ipv4(b, total, &v);
        CHECK("header length from IHL", rc == 0 && v.header == h && v.total == total && v.protocol == 6 && v.src == s && v.dst == d, "ihl %u: rc %d header %zu total %u proto %u src 0x%08x dst 0x%08x (want header %zu)", ihl, rc, v.header, v.total, v.protocol, v.src, v.dst, h);
    }

    {
        ip(b, 20, 4, 5, 20, 0, 17, 0x80000001u, 0xfffffffeu);
        memset(&v, 0, sizeof v);
        parse_ipv4(b, 20, &v);
        CHECK("addresses with the high bit set", v.src == 0x80000001u && v.dst == 0xfffffffeu, "src 0x%08x dst 0x%08x: cast each byte to uint32_t before shifting", v.src, v.dst);
    }

    {
        ip(b, 60, 4, 5, 28, 0, 17, 1, 2);
        memset(&v, 0, sizeof v);
        int rc = parse_ipv4(b, 60, &v);
        CHECK("bytes after total are padding", rc == 0 && v.total == 28 && v.header == 20, "total 28 in a 60-byte span: rc %d total %u", rc, v.total);
    }

    {
        static const unsigned flags[] = {0x0000, 0x4000, 0x8000, 0xc000, 0x2000, 0x0001, 0x1fff, 0x4001, 0x6000};
        static const int want[] =       {0,      0,      0,      0,      1,      1,      1,      1,      1};
        int bad = -1;
        for (size_t i = 0; i < sizeof flags / sizeof flags[0]; i++) {
            ip(b, 20, 4, 5, 20, flags[i], 6, 1, 2);
            memset(&v, 0, sizeof v);
            if (parse_ipv4(b, 20, &v) != 0 || v.fragmented != want[i]) { bad = (int)i; break; }
        }
        CHECK("fragment bits", bad < 0, "flags/offset 0x%04x: expected fragmented=%d (0x3fff keeps MF and the offset, not DF or the reserved bit)", bad < 0 ? 0 : flags[bad], bad < 0 ? 0 : want[bad]);
    }

    {
        int r6, r0, r15;
        ip(b, 20, 6, 5, 20, 0, 6, 1, 2); r6 = parse_ipv4(b, 20, &v);
        ip(b, 20, 0, 5, 20, 0, 6, 1, 2); r0 = parse_ipv4(b, 20, &v);
        ip(b, 20, 15, 5, 20, 0, 6, 1, 2); r15 = parse_ipv4(b, 20, &v);
        CHECK("version must be 4", r6 == -1 && r0 == -1 && r15 == -1, "versions 6, 0 and 15 must be rejected, got %d %d %d", r6, r0, r15);
    }

    {
        int low, deep;
        ip(b, 20, 4, 4, 20, 0, 6, 1, 2); low = parse_ipv4(b, 20, &v);
        ip(b, 20, 4, 0, 20, 0, 6, 1, 2); deep = parse_ipv4(b, 20, &v);
        CHECK("IHL below 5 is rejected", low == -1 && deep == -1, "IHL 4 (16 bytes) and IHL 0 cannot hold a 20-byte header, got %d %d", low, deep);
    }

    {
        int a, ok;
        ip(b, 40, 4, 15, 60, 0, 6, 1, 2); a = parse_ipv4(b, 40, &v);
        ip(b, 60, 4, 15, 60, 0, 6, 1, 2); ok = parse_ipv4(b, 60, &v);
        CHECK("IHL larger than the span", a == -1 && ok == 0 && v.header == 60, "IHL 15 needs 60 bytes: 40 supplied gave %d (want -1), 60 supplied gave %d (want 0)", a, ok);
    }

    {
        int t19, t20, beyond, exact;
        ip(b, 40, 4, 5, 19, 0, 6, 1, 2); t19 = parse_ipv4(b, 40, &v);
        ip(b, 40, 4, 5, 20, 0, 6, 1, 2); t20 = parse_ipv4(b, 40, &v);
        ip(b, 40, 4, 5, 41, 0, 6, 1, 2); beyond = parse_ipv4(b, 40, &v);
        ip(b, 40, 4, 5, 40, 0, 6, 1, 2); exact = parse_ipv4(b, 40, &v);
        CHECK("total length bounds", t19 == -1 && t20 == 0 && beyond == -1 && exact == 0, "total 19 (below header): %d, total 20: %d, total 41 in 40 bytes: %d, total 40: %d (want -1 0 -1 0)", t19, t20, beyond, exact);
    }

    {
        int short_span, one;
        ip(b, 20, 4, 5, 20, 0, 6, 1, 2);
        short_span = parse_ipv4(b, 19, &v);
        one = parse_ipv4(b, 0, &v);
        CHECK("fewer than 20 bytes", short_span == -1 && one == -1 && parse_ipv4(NULL, 20, &v) == -1 && parse_ipv4(b, 20, NULL) == -1, "n = 19 and n = 0 and NULL pointers must all return -1");
    }

    {
        v = sentinel;
        ip(b, 20, 4, 5, 99, 0, 6, 1, 2);
        parse_ipv4(b, 20, &v);
        CHECK("failure leaves out unchanged", !memcmp(&v, &sentinel, sizeof v), "a failed call wrote to *out");
    }
    return 0;
}
