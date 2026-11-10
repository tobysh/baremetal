/* Index of the first byte where a and b differ, or -1. */
static long first_diff(const uint8_t *a, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; i++)
        if (a[i] != b[i]) return (long)i;
    return -1;
}

void kmain(void) {
    static uint8_t pattern[512], back[512], zeros[512];
    uint8_t a[6] = { 4, 8, 15, 16, 23, 42 }, b[6];
    long d;
    int r;

    for (int i = 0; i < 512; i++) pattern[i] = (uint8_t)(i * 7 + 3);

    memset(disk, 0xEE, sizeof disk);
    disk_init();
    memset(back, 0x55, sizeof back);
    r = disk_read(0, back, 512);
    d = first_diff(back, zeros, 512);
    CHECK("disk_init zeroes all 512 bytes and the whole disk can be read", r == 0 && d < 0,
          "disk_read(0, 512) returned %d; first non-zero byte at offset %ld", r, d);

    r = disk_write(123, a, 6);
    CHECK("disk_write inside the disk returns 0", r == 0, "disk_write(123, 6 bytes) returned %d", r);
    memset(b, 0, sizeof b);
    r = disk_read(123, b, 6);
    d = first_diff(a, b, 6);
    CHECK("disk_read returns the bytes written at the same offset", r == 0 && d < 0,
          "disk_read returned %d; byte %ld is %u, expected %u", r, d, d < 0 ? 0 : b[d], d < 0 ? 0 : a[d]);
    r = disk_read(124, b, 2);
    CHECK("reads at a different offset see the right bytes", r == 0 && b[0] == 8 && b[1] == 15,
          "disk_read(124, 2) gave %u %u, expected 8 15", b[0], b[1]);
    r = disk_read(122, b, 1);
    CHECK("bytes around a write are untouched", r == 0 && b[0] == 0, "byte 122 is %u, expected 0", b[0]);

    r = disk_write(0, pattern, 512);
    memset(back, 0, sizeof back);
    int r2 = disk_read(0, back, 512);
    d = first_diff(pattern, back, 512);
    CHECK("a full 512-byte write and read round-trips", r == 0 && r2 == 0 && d < 0,
          "write %d, read %d, first difference at offset %ld", r, r2, d);

    r = disk_write(511, a, 1);
    int r3 = disk_read(511, b, 1);
    CHECK("the last byte (offset 511, length 1) is writable", r == 0 && r3 == 0 && b[0] == 4,
          "write returned %d, read returned %d, byte is %u", r, r3, b[0]);

    r = disk_write(512, a, 0);
    r2 = disk_read(512, b, 0);
    CHECK("zero-length requests at offset 512 succeed", r == 0 && r2 == 0, "write returned %d, read returned %d", r, r2);
    r = disk_read(0, b, 0);
    CHECK("a zero-length read at offset 0 succeeds", r == 0, "returned %d", r);

    disk_write(504, pattern, 8);
    r = disk_write(510, a, 6);
    disk_read(504, b, 6);
    disk_read(510, back, 2);
    CHECK("a write running past the end returns -1", r == -1, "disk_write(510, 6 bytes) returned %d", r);
    CHECK("a failed write changes nothing on the disk", back[0] == pattern[6] && back[1] == pattern[7],
          "bytes 510-511 are %u %u, expected %u %u (unchanged)", back[0], back[1], pattern[6], pattern[7]);

    memset(b, 0x77, sizeof b);
    r = disk_read(509, b, 4);
    CHECK("a read running past the end returns -1", r == -1, "disk_read(509, 4) returned %d", r);
    CHECK("a failed read leaves the buffer untouched", b[0] == 0x77 && b[3] == 0x77,
          "buffer bytes are %u .. %u, expected 119 (unchanged)", b[0], b[3]);

    r = disk_read(512, b, 1);
    CHECK("reading 1 byte at offset 512 fails", r == -1, "returned %d", r);
    r = disk_write(513, a, 0);
    CHECK("offset 513 fails even with length 0", r == -1, "disk_write(513, 0) returned %d", r);
    r = disk_read((size_t)-2, b, 6);
    CHECK("a huge offset is rejected", r == -1, "disk_read((size_t)-2, 6) returned %d", r);

    disk_write(10, pattern, 8);
    r = disk_write(10, a, (size_t)-5);
    disk_read(10, b, 6);
    d = first_diff(b, pattern, 6);
    CHECK("offset + len wrapping around 2^64 is rejected, not copied", r == -1 && d < 0,
          "disk_write(10, len = (size_t)-5) returned %d; disk changed at offset %ld", r, d < 0 ? -1 : 10 + d);
    r = disk_read(1, back, (size_t)-1);
    CHECK("a huge length on a read is rejected", r == -1, "disk_read(1, len = (size_t)-1) returned %d", r);

    disk_init();
    r = disk_read(0, back, 512);
    d = first_diff(back, zeros, 512);
    CHECK("disk_init zeroes a disk that was written to", r == 0 && d < 0, "byte %ld is not zero", d);
}
