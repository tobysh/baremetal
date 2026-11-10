static uint32_t st = 31;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static void w16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void w32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (24 - 8 * i)); }

/* A well-formed message with caller-chosen format fields. */
static void arp(uint8_t *b, unsigned htype, unsigned ptype, unsigned hlen, unsigned plen, unsigned op, uint32_t sip, uint32_t tip)
{
    memset(b, 0, 28);
    w16(b, htype); w16(b + 2, ptype); b[4] = (uint8_t)hlen; b[5] = (uint8_t)plen; w16(b + 6, op);
    for (int i = 8; i < 14; i++) b[i] = (uint8_t)rnd();       /* sender MAC */
    w32(b + 14, sip);
    w32(b + 24, tip);
}

int main(void)
{
    uint8_t b[64];
    ARP a;
    ARP sentinel;
    memset(&sentinel, 0x5a, sizeof sentinel);

    for (unsigned op = 1; op <= 2; op++) {
        uint32_t s = 0xc0000200u | (rnd() & 0xff), t = 0xc6336400u | (rnd() & 0xff);
        arp(b, 1, 0x0800, 6, 4, op, s, t);
        memset(&a, 0, sizeof a);
        int rc = parse_arp(b, 28, &a);
        CHECK(op == 1 ? "request is decoded" : "reply is decoded", rc == 0 && a.opcode == op && a.sender == s && a.target == t, "op %u: rc %d opcode %u sender 0x%08x target 0x%08x (want 0x%08x and 0x%08x)", op, rc, a.opcode, a.sender, a.target, s, t);
    }

    {
        arp(b, 1, 0x0800, 6, 4, 1, 0xff00007fu, 0x80ffff01u);
        memset(&a, 0, sizeof a);
        int rc = parse_arp(b, 28, &a);
        CHECK("addresses with the high bit set", rc == 0 && a.sender == 0xff00007fu && a.target == 0x80ffff01u, "sender 0x%08x target 0x%08x: cast each byte to uint32_t before shifting", a.sender, a.target);
    }

    {
        arp(b, 1, 0x0800, 6, 4, 2, 0x01020304u, 0x0a0b0c0du);
        for (int i = 28; i < 46; i++) b[i] = (uint8_t)rnd();
        memset(&a, 0, sizeof a);
        int rc = parse_arp(b, 46, &a);
        CHECK("link padding is allowed", rc == 0 && a.sender == 0x01020304u && a.target == 0x0a0b0c0du, "18 trailing bytes after a complete message must be ignored, rc %d", rc);
    }

    {
        arp(b, 1, 0x0800, 6, 4, 1, 1, 2);
        int ok = parse_arp(b, 28, &a);
        int shorter = parse_arp(b, 27, &a);
        CHECK("28 bytes is the minimum", ok == 0 && shorter == -1, "28 bytes: %d (want 0), 27 bytes: %d (want -1)", ok, shorter);
    }

    {
        int r[4];
        arp(b, 6, 0x0800, 6, 4, 1, 1, 2); r[0] = parse_arp(b, 28, &a);
        arp(b, 1, 0x86dd, 6, 4, 1, 1, 2); r[1] = parse_arp(b, 28, &a);
        arp(b, 1, 0x0800, 8, 4, 1, 1, 2); r[2] = parse_arp(b, 28, &a);
        arp(b, 1, 0x0800, 6, 16, 1, 1, 2); r[3] = parse_arp(b, 28, &a);
        CHECK("every format field is validated", r[0] == -1 && r[1] == -1 && r[2] == -1 && r[3] == -1, "htype 6, ptype 0x86dd, hlen 8 and plen 16 must all be rejected, got %d %d %d %d", r[0], r[1], r[2], r[3]);
    }

    {
        int r[4];
        arp(b, 1, 0x0800, 6, 4, 0, 1, 2); r[0] = parse_arp(b, 28, &a);
        arp(b, 1, 0x0800, 6, 4, 3, 1, 2); r[1] = parse_arp(b, 28, &a);
        arp(b, 1, 0x0800, 6, 4, 0x0100, 1, 2); r[2] = parse_arp(b, 28, &a);
        arp(b, 1, 0x0800, 6, 4, 0xffff, 1, 2); r[3] = parse_arp(b, 28, &a);
        CHECK("only operations 1 and 2", r[0] == -1 && r[1] == -1 && r[2] == -1 && r[3] == -1, "operations 0, 3, 256 and 65535 must be rejected, got %d %d %d %d", r[0], r[1], r[2], r[3]);
    }

    {
        arp(b, 1, 0x0800, 6, 4, 1, 1, 2);
        w16(b, 0x0100);     /* htype bytes swapped: 256 */
        CHECK("format fields are big-endian", parse_arp(b, 28, &a) == -1, "01 00 is 256, not 1");
    }

    {
        a = sentinel;
        arp(b, 1, 0x0800, 6, 4, 9, 1, 2);
        parse_arp(b, 28, &a);
        int same = !memcmp(&a, &sentinel, sizeof a);
        parse_arp(b, 10, &a);
        CHECK("failure leaves out unchanged", same && !memcmp(&a, &sentinel, sizeof a), "a failed call wrote to *out");
    }

    CHECK("NULL and empty input", parse_arp(NULL, 28, &a) == -1 && parse_arp(b, 28, NULL) == -1 && parse_arp(b, 0, &a) == -1, "NULL pointers and n == 0 must return -1");
    return 0;
}
