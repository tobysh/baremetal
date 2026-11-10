static uint32_t st = 11;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

/* Builds a frame with random MACs; vlan != 0 inserts a tag. Returns the header length. */
static size_t frame(uint8_t *b, unsigned type, int vlan, unsigned inner)
{
    for (int i = 0; i < 12; i++)
        b[i] = (uint8_t)rnd();
    size_t off = 12;
    if (vlan) {
        b[off++] = 0x81; b[off++] = 0x00;
        b[off++] = (uint8_t)rnd(); b[off++] = (uint8_t)rnd();   /* TCI */
        b[off++] = (uint8_t)(inner >> 8); b[off++] = (uint8_t)inner;
    } else {
        b[off++] = (uint8_t)(type >> 8); b[off++] = (uint8_t)type;
    }
    return off;
}

int main(void)
{
    uint8_t b[64];
    Ethernet e;
    static const unsigned types[] = {0x0800, 0x0806, 0x86dd, 0x88b5, 0xff00, 0x0600};

    for (size_t i = 0; i < sizeof types / sizeof types[0]; i++) {
        size_t h = frame(b, types[i], 0, 0);
        memset(&e, 0, sizeof e);
        int rc = parse_ethernet(b, h, &e);
        CHECK("untagged type and offset", rc == 0 && e.type == types[i] && e.payload == 14, "type 0x%04x: rc %d type 0x%04x payload %zu (want payload 14)", types[i], rc, e.type, e.payload);
    }

    {
        size_t h = frame(b, 0, 0, 0);
        b[12] = 0x08; b[13] = 0x00;
        memset(&e, 0, sizeof e);
        parse_ethernet(b, h, &e);
        CHECK("addresses are copied byte for byte", !memcmp(e.dst, b, 6) && !memcmp(e.src, b + 6, 6), "dst and src must be the first and second six bytes, unchanged");
    }

    for (size_t i = 0; i < sizeof types / sizeof types[0]; i++) {
        size_t h = frame(b, 0, 1, types[i]);
        memset(&e, 0, sizeof e);
        int rc = parse_ethernet(b, h, &e);
        CHECK("one VLAN tag is unwrapped", rc == 0 && e.type == types[i] && e.payload == 18 && !memcmp(e.src, b + 6, 6), "inner type 0x%04x: rc %d type 0x%04x payload %zu (want payload 18)", types[i], rc, e.type, e.payload);
    }

    {
        size_t h = frame(b, 0, 1, 0x8100);
        memset(&e, 0, sizeof e);
        int rc = parse_ethernet(b, h, &e);
        CHECK("a second tag is not unwrapped", rc == 0 && e.type == 0x8100 && e.payload == 18, "inner 0x8100 must be returned as the type with payload 18, got rc %d type 0x%04x payload %zu", rc, e.type, e.payload);
    }

    {
        size_t h = frame(b, 0x0800, 0, 0);
        for (size_t i = 0; i < 20; i++)
            b[h + i] = (uint8_t)rnd();
        memset(&e, 0, sizeof e);
        int rc = parse_ethernet(b, h + 20, &e);
        CHECK("trailing bytes are ignored", rc == 0 && e.payload == 14 && e.type == 0x0800, "padding after the header must not change the result");
    }

    {
        size_t h = frame(b, 0x0800, 0, 0);
        int shortest = parse_ethernet(b, 14, &e);
        int one_less = parse_ethernet(b, 13, &e);
        CHECK("14 bytes is the minimum", shortest == 0 && one_less == -1, "14 bytes should succeed (got %d) and 13 should fail (got %d)", shortest, one_less);
        (void)h;
    }

    {
        frame(b, 0, 1, 0x0800);
        int ok = parse_ethernet(b, 18, &e);
        int bad = parse_ethernet(b, 17, &e);
        int bare = parse_ethernet(b, 14, &e);
        CHECK("tagged frames need 18 bytes", ok == 0 && bad == -1 && bare == -1, "18 bytes: %d (want 0), 17 bytes: %d, 14 bytes: %d (want -1 for both)", ok, bad, bare);
    }

    {
        Ethernet sentinel;
        memset(&sentinel, 0x5a, sizeof sentinel);
        e = sentinel;
        frame(b, 0, 1, 0x0800);
        parse_ethernet(b, 17, &e);
        int same = !memcmp(&e, &sentinel, sizeof e);
        parse_ethernet(b, 3, &e);
        CHECK("failure leaves out unchanged", same && !memcmp(&e, &sentinel, sizeof e), "a failed call wrote to *out");
    }

    CHECK("NULL arguments", parse_ethernet(NULL, 14, &e) == -1 && parse_ethernet(b, 14, NULL) == -1 && parse_ethernet(b, 0, &e) == -1, "NULL pointers and n == 0 must return -1");
    return 0;
}
