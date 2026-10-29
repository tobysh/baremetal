int main(void) {
    uint8_t b[8];
    memset(b, 0xAA, sizeof b);
    put_u16(b + 1, 0x1234);
    CHECK("put_u16 writes the high byte first", b[1] == 0x12 && b[2] == 0x34, "got %02x %02x", b[1], b[2]);
    CHECK("put_u16 writes only 2 bytes", b[0] == 0xAA && b[3] == 0xAA, "neighbours were touched");
    put_u16(b, 0x00ff);
    CHECK("put_u16 handles a small value", b[0] == 0x00 && b[1] == 0xff, "got %02x %02x", b[0], b[1]);

    memset(b, 0xAA, sizeof b);
    put_u32(b + 2, 0xA1B2C3D4u);
    CHECK("put_u32 writes big-endian", b[2] == 0xA1 && b[3] == 0xB2 && b[4] == 0xC3 && b[5] == 0xD4, "got %02x %02x %02x %02x", b[2], b[3], b[4], b[5]);
    CHECK("put_u32 writes only 4 bytes", b[1] == 0xAA && b[6] == 0xAA, "neighbours were touched");
    put_u32(b, 1);
    CHECK("put_u32(1) is 00 00 00 01", b[0] == 0 && b[1] == 0 && b[2] == 0 && b[3] == 1, "got %02x %02x %02x %02x", b[0], b[1], b[2], b[3]);

    uint8_t in16[2] = {0xBE, 0xEF};
    CHECK("get_u16 reads big-endian", get_u16(in16) == 0xBEEF, "got 0x%x", get_u16(in16));
    uint8_t hi[2] = {0xFF, 0xFE};
    CHECK("get_u16 does not sign-extend", get_u16(hi) == 0xFFFE, "got 0x%x", get_u16(hi));
    uint8_t in32[4] = {0x80, 0x00, 0x00, 0x01};
    CHECK("get_u32 reads big-endian", get_u32(in32) == 0x80000001u, "got 0x%x", get_u32(in32));
    uint8_t ff[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    CHECK("get_u32 handles 0xFFFFFFFF", get_u32(ff) == 0xFFFFFFFFu, "got 0x%x", get_u32(ff));

    CHECK("swap32 reverses the bytes", swap32(0x11223344u) == 0x44332211u, "got 0x%x", swap32(0x11223344u));
    CHECK("swap32 of 0xFF is 0xFF000000", swap32(0xFFu) == 0xFF000000u, "got 0x%x", swap32(0xFFu));
    CHECK("swap32 twice is the identity", swap32(swap32(0xCAFEBABEu)) == 0xCAFEBABEu, "not an involution");

    uint32_t x = 12345;
    int ok = 1;
    for (int i = 0; i < 200; i++) {
        x = x * 1103515245u + 12345u;
        uint8_t t[4];
        put_u32(t, x);
        uint32_t back = get_u32(t);
        uint8_t t2[2];
        put_u16(t2, (uint16_t)(x >> 8));
        if (back != x || get_u16(t2) != (uint16_t)(x >> 8) || t[0] != (uint8_t)(x >> 24) || t[3] != (uint8_t)x) ok = 0;
    }
    CHECK("put and get round-trip 200 pseudo-random values", ok, "a round trip failed");
    return 0;
}
