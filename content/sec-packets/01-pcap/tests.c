/* Synthetic pcap files are built in memory with a seeded generator, in both byte orders. */
static uint32_t st = 77;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

static void put(uint8_t *p, int bytes, int big, uint32_t v)
{
    for (int i = 0; i < bytes; i++)
        p[big ? bytes - 1 - i : i] = (uint8_t)(v >> (8 * i));
}

/* Writes the 24-byte global header and returns its length. */
static size_t hdr(uint8_t *b, int big, uint32_t snap, uint32_t link)
{
    memset(b, 0, 24);
    memcpy(b, big ? "\xa1\xb2\xc3\xd4" : "\xd4\xc3\xb2\xa1", 4);
    put(b + 4, 2, big, 2);
    put(b + 6, 2, big, 4);
    put(b + 16, 4, big, snap);
    put(b + 20, 4, big, link);
    return 24;
}

/* Appends one record (header plus `cap` filler bytes) and returns the new length. */
static size_t rec(uint8_t *b, size_t off, int big, uint32_t usec, uint32_t cap, uint32_t orig)
{
    put(b + off, 4, big, 1700000000u + (uint32_t)off);
    put(b + off + 4, 4, big, usec);
    put(b + off + 8, 4, big, cap);
    put(b + off + 12, 4, big, orig);
    for (uint32_t i = 0; i < cap; i++)
        b[off + 16 + i] = (uint8_t)rnd();
    return off + 16 + cap;
}

int main(void)
{
    static uint8_t b[4096];
    size_t n;

    for (int big = 0; big < 2; big++) {
        n = hdr(b, big, 65535, 1);
        CHECK("header-only file has zero records", pcap_count(b, n) == 0, "%s-endian header only: expected 0, got %d", big ? "big" : "little", pcap_count(b, n));
    }

    for (int big = 0; big < 2; big++) {
        int want = 2 + (int)(rnd() % 4);
        n = hdr(b, big, 200, 1);
        for (int i = 0; i < want; i++)
            n = rec(b, n, big, rnd() % 1000000, 1 + rnd() % 200, 300);
        int got = pcap_count(b, n);
        CHECK("counts records in either byte order", got == want, "%s-endian file with %d records returned %d (decode every integer in the byte order the magic selects)", big ? "big" : "little", want, got);
    }

    n = hdr(b, 0, 60, 1);
    n = rec(b, n, 0, 999999, 60, 1514);
    CHECK("truncated capture is valid", pcap_count(b, n) == 1, "caplen 60 below origlen 1514 with snaplen 60 is ordinary truncation, got %d", pcap_count(b, n));

    n = hdr(b, 1, 100, 1);
    n = rec(b, n, 1, 0, 100, 100);
    CHECK("caplen equal to snaplen is allowed", pcap_count(b, n) == 1, "caplen == snaplen == origlen must pass, got %d", pcap_count(b, n));
    n = hdr(b, 1, 100, 1);
    n = rec(b, n, 1, 0, 101, 200);
    CHECK("caplen above snaplen is rejected", pcap_count(b, n) == -1, "caplen 101 exceeds snaplen 100");

    n = hdr(b, 0, 100, 1);
    n = rec(b, n, 0, 0, 40, 30);
    CHECK("caplen above origlen is rejected", pcap_count(b, n) == -1, "caplen 40 exceeds origlen 30");

    n = hdr(b, 0, 100, 1);
    n = rec(b, n, 0, 1000000, 10, 10);
    CHECK("microseconds must be below one million", pcap_count(b, n) == -1, "usec 1000000 is not a valid fraction of a second");

    n = hdr(b, 0, 100, 1);
    n = rec(b, n, 0, 5, 10, 10);
    size_t whole = n;
    CHECK("short final record body", pcap_count(b, whole - 1) == -1, "record claims 10 bytes but only 9 are present");
    CHECK("partial trailing record header", pcap_count(b, whole + 15) == -1, "15 spare bytes after the last record are a partial header, not padding");
    memset(b + whole, 0, 16);
    CHECK("zero-length record is a record", pcap_count(b, whole + 16) == 2, "a record header with caplen 0 and origlen 0 is complete, expected 2 got %d", pcap_count(b, whole + 16));

    n = hdr(b, 0, 100, 1);
    memcpy(b, "\x4d\x3c\xb2\xa1", 4);
    CHECK("nanosecond magic is unsupported", pcap_count(b, n) == -1, "this lesson accepts microsecond pcap only");
    memcpy(b, "\x0a\x0d\x0d\x0a", 4);
    CHECK("pcapng block is unsupported", pcap_count(b, n) == -1, "pcapng starts with 0a 0d 0d 0a and is not classic pcap");

    n = hdr(b, 0, 100, 1);
    put(b + 6, 2, 0, 3);
    int minor3 = pcap_count(b, n);
    n = hdr(b, 1, 100, 1);
    put(b + 4, 2, 1, 3);
    CHECK("version must be exactly 2.4", minor3 == -1 && pcap_count(b, n) == -1, "versions 2.3 and 3.4 must both be rejected");

    n = hdr(b, 0, 0, 1);
    CHECK("snaplen zero is rejected", pcap_count(b, n) == -1, "snaplen 0 cannot hold any frame");
    n = hdr(b, 1, 100, 101);
    CHECK("link type must be Ethernet", pcap_count(b, n) == -1, "link type 101 (raw IP) is outside this lesson");

    /* A little-endian magic followed by big-endian fields: decoding in the wrong order must fail. */
    n = hdr(b, 1, 100, 1);
    memcpy(b, "\xd4\xc3\xb2\xa1", 4);
    CHECK("fields must follow the magic's byte order", pcap_count(b, n) == -1, "magic says little-endian but version bytes are 00 02, which reads as 512");

    n = hdr(b, 0, 100, 1);
    CHECK("short global header", pcap_count(b, 23) == -1 && pcap_count(b, 0) == -1 && pcap_count(NULL, 24) == -1, "n < 24 or a NULL pointer must return -1");
    return 0;
}
