/* Index of the first i where got[i] != want[i], or -1. */
static int mismatch(const uint8_t *got, const uint8_t *want, int n) {
    for (int i = 0; i < n; i++)
        if (got[i] != want[i]) return i;
    return -1;
}

void kmain(void) {
    Pipe p, q;
    uint8_t a[24], b[32];
    int n, m, k;

    for (int i = 0; i < 24; i++) a[i] = (uint8_t)(i + 1);
    memset(&p, 0xCC, sizeof p);
    pipe_init(&p);

    n = pipe_read(&p, b, 1);
    CHECK("reading an empty pipe with the writer open returns -1 (try again)", n == -1, "pipe_read returned %d", n);
    n = pipe_read(&p, b, 0);
    CHECK("a zero-length read returns 0", n == 0, "pipe_read(p, b, 0) returned %d", n);
    n = pipe_write(&p, a, 0);
    CHECK("a zero-length write to an open pipe returns 0", n == 0, "pipe_write(p, a, 0) returned %d", n);

    n = pipe_write(&p, a, 5);
    CHECK("a small write is accepted in full", n == 5, "pipe_write of 5 bytes returned %d", n);
    memset(b, 0, sizeof b);
    n = pipe_read(&p, b, 5);
    k = mismatch(b, a, 5);
    CHECK("bytes come out in the order they went in", n == 5 && k < 0,
          "pipe_read returned %d; byte %d is %u, expected %u", n, k, k < 0 ? 0 : b[k], k < 0 ? 0 : a[k]);
    n = pipe_read(&p, b, 1);
    CHECK("the pipe is empty again after reading everything", n == -1, "pipe_read returned %d, expected -1", n);

    pipe_init(&p);
    n = pipe_write(&p, a, 20);
    CHECK("a write larger than the space left is short: 16 of 20 bytes", n == 16, "pipe_write of 20 bytes returned %d", n);
    n = pipe_write(&p, a, 1);
    CHECK("a write to a full pipe returns 0", n == 0, "returned %d", n);

    memset(b, 0, sizeof b);
    n = pipe_read(&p, b, 10);
    k = mismatch(b, a, 10);
    CHECK("a partial read returns the requested 10 oldest bytes", n == 10 && k < 0,
          "pipe_read returned %d; byte %d is %u, expected %u", n, k, k < 0 ? 0 : b[k], k < 0 ? 0 : a[k]);

    n = pipe_write(&p, a + 16, 8);
    CHECK("after a read, the freed space wraps around the end of the ring", n == 8, "pipe_write of 8 bytes returned %d", n);
    n = pipe_write(&p, a, 4);
    CHECK("the wrapped pipe is limited to 16 bytes in total", n == 2, "pipe_write of 4 more bytes returned %d, expected 2", n);

    memset(b, 0, sizeof b);
    n = pipe_read(&p, b, 32);
    uint8_t want[16];
    memcpy(want, a + 10, 14);
    want[14] = a[0];
    want[15] = a[1];
    k = mismatch(b, want, 16);
    CHECK("a large read returns everything buffered (16 bytes), in order across the wrap", n == 16 && k < 0,
          "pipe_read returned %d; byte %d is %u, expected %u", n, k, k < 0 ? 0 : b[k], k < 0 ? 0 : want[k]);

    pipe_init(&p);
    pipe_write(&p, a, 3);
    pipe_close_writer(&p);
    memset(b, 0, sizeof b);
    n = pipe_read(&p, b, 2);
    m = pipe_read(&p, b + 2, 8);
    k = mismatch(b, a, 3);
    CHECK("bytes written before the close are still delivered", n == 2 && m == 1 && k < 0,
          "reads after close returned %d and %d (expected 2 and 1); byte %d wrong", n, m, k);
    n = pipe_read(&p, b, 4);
    CHECK("an empty pipe with the writer closed returns 0 (end of file)", n == 0, "pipe_read returned %d", n);
    n = pipe_read(&p, b, 4);
    CHECK("end of file is reported again on every later read", n == 0, "pipe_read returned %d", n);
    n = pipe_write(&p, a, 1);
    CHECK("writing after close returns -1", n == -1, "pipe_write returned %d", n);
    n = pipe_write(&p, a, 0);
    CHECK("even a zero-length write after close returns -1", n == -1, "pipe_write(p, a, 0) returned %d", n);

    memset(&q, 0x11, sizeof q);
    pipe_init(&q);
    pipe_write(&q, a, 4);
    n = pipe_read(&p, b, 1);
    m = pipe_read(&q, b, 8);
    CHECK("two pipes are independent", n == 0 && m == 4, "closed pipe read %d (expected 0), other pipe read %d (expected 4)", n, m);
    pipe_init(&p);
    n = pipe_write(&p, a, 2);
    CHECK("pipe_init reopens a closed pipe", n == 2, "pipe_write after pipe_init returned %d", n);
}
