static uint32_t st = 123;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static void w16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }

/* `n` bytes with the given ports and declared length; the rest is random. */
static void dgram(uint8_t *b, size_t n, unsigned sport, unsigned dport, unsigned len)
{
    for (size_t i = 0; i < n; i++)
        b[i] = (uint8_t)rnd();
    w16(b, sport); w16(b + 2, dport); w16(b + 4, len);
}

int main(void)
{
    static uint8_t b[70000];
    UDP u;
    UDP sentinel;
    memset(&sentinel, 0x5a, sizeof sentinel);

    {
        dgram(b, 8, 40001, 53, 8);
        memset(&u, 0, sizeof u);
        int rc = parse_udp(b, 8, &u);
        CHECK("empty datagram", rc == 0 && u.src == 40001 && u.dst == 53 && u.payload == 0, "length 8: rc %d src %u dst %u payload %zu", rc, u.src, u.dst, u.payload);
    }

    {
        unsigned extra = 1 + rnd() % 40;
        dgram(b, 8 + extra, 53, 50000, 8 + extra);
        memset(&u, 0, sizeof u);
        int rc = parse_udp(b, 8 + extra, &u);
        CHECK("payload is length minus 8", rc == 0 && u.src == 53 && u.dst == 50000 && u.payload == extra, "length %u: payload %zu, want %u", 8 + extra, u.payload, extra);
    }

    {
        unsigned extra = 3 + rnd() % 20, pad = 1 + rnd() % 20;
        dgram(b, 8 + extra + pad, 123, 65535, 8 + extra);
        memset(&u, 0, sizeof u);
        int rc = parse_udp(b, 8 + extra + pad, &u);
        CHECK("trailing bytes are not payload", rc == 0 && u.payload == extra, "length says %u payload bytes but %zu were reported (use the declared length, not n - 8)", extra, u.payload);
    }

    {
        dgram(b, 8, 1, 2, 7);
        int r7 = parse_udp(b, 8, &u);
        dgram(b, 8, 1, 2, 0);
        int r0 = parse_udp(b, 8, &u);
        dgram(b, 20, 1, 2, 1);
        int r1 = parse_udp(b, 20, &u);
        CHECK("length below the header size", r7 == -1 && r0 == -1 && r1 == -1, "lengths 7, 0 and 1 cannot cover an 8-byte header, got %d %d %d", r7, r0, r1);
    }

    {
        dgram(b, 20, 1, 2, 21);
        int over = parse_udp(b, 20, &u);
        dgram(b, 20, 1, 2, 20);
        int exact = parse_udp(b, 20, &u);
        CHECK("length beyond the captured bytes", over == -1 && exact == 0 && u.payload == 12, "length 21 in 20 bytes: %d (want -1); length 20: %d (want 0)", over, exact);
    }

    {
        dgram(b, 8, 1, 2, 0xffff);
        int tiny = parse_udp(b, 8, &u);
        dgram(b, 65535, 7, 9, 65535);
        memset(&u, 0, sizeof u);
        int full = parse_udp(b, 65535, &u);
        CHECK("maximum length", tiny == -1 && full == 0 && u.payload == 65527, "65535 in 8 bytes: %d (want -1); 65535 in 65535 bytes: %d payload %zu (want 65527)", tiny, full, u.payload);
    }

    {
        dgram(b, 12, 0x0102, 0x0304, 12);
        memset(&u, 0, sizeof u);
        parse_udp(b, 12, &u);
        CHECK("ports and length are big-endian", u.src == 0x0102 && u.dst == 0x0304 && u.payload == 4, "src 0x%04x dst 0x%04x payload %zu", u.src, u.dst, u.payload);
    }

    {
        dgram(b, 8, 1, 2, 8);
        CHECK("fewer than 8 bytes", parse_udp(b, 7, &u) == -1 && parse_udp(b, 0, &u) == -1, "a 7-byte span has no length field");
    }

    {
        u = sentinel;
        dgram(b, 8, 1, 2, 99);
        parse_udp(b, 8, &u);
        int same = !memcmp(&u, &sentinel, sizeof u);
        parse_udp(b, 3, &u);
        CHECK("failure leaves out unchanged", same && !memcmp(&u, &sentinel, sizeof u), "a failed call wrote to *out");
    }

    CHECK("NULL arguments", parse_udp(NULL, 8, &u) == -1 && parse_udp(b, 8, NULL) == -1, "NULL pointers must return -1");
    return 0;
}
