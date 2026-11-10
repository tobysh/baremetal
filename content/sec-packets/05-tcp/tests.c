static uint32_t st = 99;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static void w16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void w32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (24 - 8 * i)); }

/* An n-byte segment with random option/payload bytes. `b12` is the whole byte 12. */
static void seg(uint8_t *b, size_t n, unsigned sport, unsigned dport, uint32_t seq, uint32_t ack, unsigned b12, unsigned flags)
{
    for (size_t i = 0; i < n; i++)
        b[i] = (uint8_t)rnd();
    w16(b, sport); w16(b + 2, dport); w32(b + 4, seq); w32(b + 8, ack);
    b[12] = (uint8_t)b12; b[13] = (uint8_t)flags;
}

int main(void)
{
    uint8_t b[128];
    TCP t;
    TCP sentinel;
    memset(&sentinel, 0x5a, sizeof sentinel);

    {
        seg(b, 20, 40000, 80, 1000, 0, 0x50, 0x02);
        memset(&t, 0, sizeof t);
        int rc = parse_tcp(b, 20, &t);
        CHECK("plain SYN", rc == 0 && t.src == 40000 && t.dst == 80 && t.seq == 1000 && t.ack == 0 && t.flags == 2 && t.header == 20 && t.payload == 0, "rc %d src %u dst %u seq %u ack %u flags %u header %zu payload %zu", rc, t.src, t.dst, t.seq, t.ack, t.flags, t.header, t.payload);
    }

    {
        size_t extra = 1 + rnd() % 30;
        seg(b, 20 + extra, 80, 40000, 1001, 999, 0x50, 0x18);
        memset(&t, 0, sizeof t);
        int rc = parse_tcp(b, 20 + extra, &t);
        CHECK("payload is n minus header", rc == 0 && t.header == 20 && t.payload == extra, "with %zu bytes after the header, payload was %zu", extra, t.payload);
    }

    for (unsigned words = 6; words <= 15; words += 3) {
        size_t h = words * 4;
        size_t extra = rnd() % 10;
        seg(b, h + extra, 1, 2, 0, 0, words << 4, 0x10);
        memset(&t, 0, sizeof t);
        int rc = parse_tcp(b, h + extra, &t);
        CHECK("options extend the header", rc == 0 && t.header == h && t.payload == extra, "offset %u: header %zu (want %zu) payload %zu (want %zu)", words, t.header, h, t.payload, extra);
    }

    {
        seg(b, 60, 1, 2, 0, 0, 0xf0, 0x10);
        int exact = parse_tcp(b, 60, &t);
        int shortfall = parse_tcp(b, 59, &t);
        CHECK("offset 15 needs 60 bytes", exact == 0 && t.header == 60 && t.payload == 0 && shortfall == -1, "60 bytes: %d (want 0), 59 bytes: %d (want -1)", exact, shortfall);
    }

    {
        int r4, r0;
        seg(b, 40, 1, 2, 0, 0, 0x40, 0x02); r4 = parse_tcp(b, 40, &t);
        seg(b, 40, 1, 2, 0, 0, 0x00, 0x02); r0 = parse_tcp(b, 40, &t);
        CHECK("offset below 5 is invalid", r4 == -1 && r0 == -1, "offset 4 (16 bytes) and offset 0 cannot hold the fixed header, got %d %d", r4, r0);
    }

    {
        seg(b, 20, 1, 2, 0, 0, 0x5f, 0x02);
        memset(&t, 0, sizeof t);
        int rc = parse_tcp(b, 20, &t);
        CHECK("low nibble of byte 12 is ignored", rc == 0 && t.header == 20, "byte 12 = 0x5f still means offset 5, got rc %d header %zu", rc, t.header);
    }

    {
        seg(b, 20, 0, 65535, 0xffffffffu, 0x80000000u, 0x50, 0xff);
        memset(&t, 0, sizeof t);
        int rc = parse_tcp(b, 20, &t);
        CHECK("extreme values and raw flags", rc == 0 && t.src == 0 && t.dst == 65535 && t.seq == 0xffffffffu && t.ack == 0x80000000u && t.flags == 0xff, "seq 0x%08x ack 0x%08x flags 0x%02x: keep full 32-bit values and the whole flag byte", t.seq, t.ack, t.flags);
    }

    {
        seg(b, 20, 1, 2, 0, 0, 0x50, 0x02);
        CHECK("fewer than 20 bytes", parse_tcp(b, 19, &t) == -1 && parse_tcp(b, 0, &t) == -1, "a truncated base header must return -1");
    }

    {
        t = sentinel;
        seg(b, 20, 1, 2, 0, 0, 0x60, 0x02);
        parse_tcp(b, 20, &t);
        int same = !memcmp(&t, &sentinel, sizeof t);
        parse_tcp(b, 7, &t);
        CHECK("failure leaves out unchanged", same && !memcmp(&t, &sentinel, sizeof t), "a failed call wrote to *out");
    }

    CHECK("NULL arguments", parse_tcp(NULL, 20, &t) == -1 && parse_tcp(b, 20, NULL) == -1, "NULL pointers must return -1");
    return 0;
}
