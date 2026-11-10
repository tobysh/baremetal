/* 1 if buf holds first, first + 1, ... for n bytes. */
static int run_from(const uint8_t *buf, int first, int n) {
    for (int i = 0; i < n; i++)
        if (buf[i] != (uint8_t)(first + i)) return 0;
    return 1;
}

void kmain(void) {
    uint8_t a[40], b[40];
    int x, y, s2, s3, s4, n, m;

    memset(fds, 0x7F, sizeof fds);
    files_init();
    x = file_open();
    memset(a, 0xFF, sizeof a);
    n = file_read(x, a, 32);
    CHECK("files_init fills the file with 0..31 and file_open gives slot 0", x == 0 && n == 32 && run_from(a, 0, 32),
          "file_open returned %d, read returned %d, a[0]=%u a[31]=%u", x, n, a[0], a[31]);

    files_init();
    x = file_open();
    y = file_open();
    s2 = file_open();
    s3 = file_open();
    s4 = file_open();
    CHECK("file_open hands out slots 0, 1, 2, 3 in order", x == 0 && y == 1 && s2 == 2 && s3 == 3,
          "got %d %d %d %d", x, y, s2, s3);
    CHECK("file_open returns -1 when all four slots are open", s4 == -1, "fifth file_open returned %d", s4);

    memset(a, 0xFF, sizeof a);
    n = file_read(x, a, 5);
    CHECK("the first read starts at offset 0", n == 5 && run_from(a, 0, 5),
          "read returned %d, bytes %u..%u, expected 5 bytes 0..4", n, a[0], a[4]);
    n = file_read(x, a, 3);
    CHECK("the next read continues where the last one stopped", n == 3 && run_from(a, 5, 3),
          "read returned %d, first byte %u, expected 3 bytes from 5", n, a[0]);
    n = file_read(y, b, 3);
    CHECK("another descriptor has its own offset, still at 0", n == 3 && run_from(b, 0, 3),
          "read on fd 1 returned %d, first byte %u, expected 0", n, b[0]);
    n = file_read(x, a, 0);
    m = file_read(x, a, 1);
    CHECK("a zero-length read returns 0 and does not move the offset", n == 0 && m == 1 && a[0] == 8,
          "zero-length read returned %d; next byte read was %u, expected 8", n, a[0]);

    memset(a, 0xFF, sizeof a);
    n = file_read(x, a, 40);
    CHECK("a read past the end is short: 23 bytes from offset 9", n == 23 && run_from(a, 9, 23),
          "read returned %d, first byte %u, last byte %u", n, a[0], n > 0 ? a[n - 1] : 0);
    n = file_read(x, a, 1);
    CHECK("at end of file, read returns 0", n == 0, "read returned %d", n);
    n = file_read(x, a, 0);
    CHECK("a zero-length read at end of file returns 0", n == 0, "read returned %d", n);

    n = file_read(-1, a, 1);
    CHECK("file_read rejects fd -1", n == -1, "returned %d", n);
    n = file_read(4, a, 1);
    CHECK("file_read rejects fd 4 (past the table)", n == -1, "returned %d", n);
    n = file_close(4);
    m = file_close(-3);
    CHECK("file_close rejects descriptors outside 0..3", n == -1 && m == -1, "file_close(4) returned %d, file_close(-3) returned %d", n, m);

    n = file_close(y);
    CHECK("file_close of an open descriptor returns 0", n == 0, "returned %d", n);
    n = file_read(y, a, 1);
    m = file_read(y, a, 0);
    CHECK("reads through a closed descriptor return -1, even zero-length ones", n == -1 && m == -1,
          "read returned %d, zero-length read returned %d", n, m);
    n = file_close(y);
    CHECK("closing a descriptor twice returns -1", n == -1, "second file_close returned %d", n);

    y = file_open();
    memset(b, 0xFF, sizeof b);
    n = file_read(y, b, 2);
    CHECK("the lowest free slot is reused and starts at offset 0", y == 1 && n == 2 && run_from(b, 0, 2),
          "file_open returned %d; read returned %d with bytes %u %u", y, n, b[0], b[1]);

    file_close(s3);
    file_close(x);
    x = file_open();
    s3 = file_open();
    CHECK("with slots 0 and 3 free, file_open returns 0 then 3", x == 0 && s3 == 3, "got %d then %d", x, s3);
    n = file_read(x, a, 4);
    CHECK("reopened slot 0 starts at offset 0 even though its last user reached EOF", n == 4 && run_from(a, 0, 4),
          "read returned %d, first byte %u", n, a[0]);

    files_init();
    x = file_open();
    CHECK("files_init closes every descriptor", x == 0, "file_open after files_init returned %d", x);
}
