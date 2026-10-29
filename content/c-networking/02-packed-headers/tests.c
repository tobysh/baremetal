int main(void) {
    Header h = {1, 5, 7, 300, 99};
    uint8_t b[16];
    memset(b, 0xCC, sizeof b);
    hdr_encode(&h, b);
    CHECK("magic is first", b[0] == 0xB4 && b[1] == 0xDE, "got %02x %02x", b[0], b[1]);
    CHECK("version and flags share a byte", b[2] == 0x15, "got 0x%02x, expected 0x15", b[2]);
    CHECK("type is byte 3", b[3] == 7, "got %d", b[3]);
    CHECK("length is big-endian at offset 4", b[4] == 0x01 && b[5] == 0x2C, "got %02x %02x", b[4], b[5]);
    CHECK("seq is big-endian at offset 6", b[6] == 0 && b[7] == 0 && b[8] == 0 && b[9] == 99, "got %02x %02x %02x %02x", b[6], b[7], b[8], b[9]);
    CHECK("encode writes exactly 10 bytes", b[10] == 0xCC, "byte 10 was changed");

    Header wide = {0xF1, 0xA3, 255, 0xFFFF, 0xDEADBEEFu};
    hdr_encode(&wide, b);
    CHECK("high bits of version and flags are masked off", b[2] == 0x13, "got 0x%02x, expected 0x13", b[2]);
    CHECK("large seq is encoded big-endian", b[6] == 0xDE && b[7] == 0xAD && b[8] == 0xBE && b[9] == 0xEF, "got %02x %02x %02x %02x", b[6], b[7], b[8], b[9]);

    Header out;
    memset(&out, 0x55, sizeof out);
    hdr_encode(&h, b);
    int r = hdr_decode(b, 10, &out);
    CHECK("decode returns 10", r == 10, "returned %d", r);
    CHECK("decode restores every field", out.version == 1 && out.flags == 5 && out.type == 7 && out.length == 300 && out.seq == 99,
          "got v%d f%d t%d len%d seq%u", out.version, out.flags, out.type, out.length, out.seq);
    r = hdr_decode(b, 16, &out);
    CHECK("extra trailing bytes are ignored", r == 10, "returned %d", r);

    Header junk;
    memset(&junk, 0x55, sizeof junk);
    Header junk2 = junk;
    r = hdr_decode(b, 9, &junk);
    CHECK("9 bytes means need more data (0)", r == 0, "returned %d", r);
    CHECK("short input leaves the header untouched", memcmp(&junk, &junk2, sizeof junk) == 0, "header was modified");
    r = hdr_decode(b, 0, &junk);
    CHECK("empty input means need more data", r == 0, "returned %d", r);
    b[1] = 0xDF;
    r = hdr_decode(b, 10, &junk);
    CHECK("bad magic returns -1", r == -1, "returned %d", r);
    CHECK("bad magic leaves the header untouched", memcmp(&junk, &junk2, sizeof junk) == 0, "header was modified");

    uint32_t x = 7;
    int ok = 1;
    for (int i = 0; i < 100; i++) {
        x = x * 1664525u + 1013904223u;
        Header a = {(uint8_t)(x & 15), (uint8_t)((x >> 4) & 15), (uint8_t)(x >> 8), (uint16_t)(x >> 3), x * 31u};
        Header c;
        uint8_t t[10];
        hdr_encode(&a, t);
        if (hdr_decode(t, 10, &c) != 10 || c.version != a.version || c.flags != a.flags || c.type != a.type || c.length != a.length || c.seq != a.seq) ok = 0;
    }
    CHECK("100 pseudo-random headers round-trip", ok, "a round trip failed");
    return 0;
}
