/* Takes frames until the pool is empty and returns how many it got. */
static unsigned drain(void) {
    unsigned n = 0;
    while (frame_take() && n < 100)
        n++;
    return n;
}

#define F(i) (BASE + 4096ULL * (i))

void kmain(void) {
    uint64_t a, b;
    unsigned n;
    int r;

    used = ~0ULL;
    frame_init();
    a = frame_take();
    CHECK("frame_init frees every frame and frame_take returns frame 0 first", a == F(0),
          "after frame_init, frame_take returned 0x%lx, expected 0x%lx", a, F(0));
    b = frame_take();
    CHECK("frame_take marks the frame it returns, so the next call returns frame 1", b == F(1),
          "second frame_take returned 0x%lx, expected 0x%lx", b, F(1));

    frame_init();
    frame_reserve(0, 2);
    a = frame_take();
    CHECK("reserved frames 0 and 1 are skipped", a == F(2),
          "frame_take returned 0x%lx, expected 0x%lx (frame 2)", a, F(2));

    frame_init();
    frame_reserve(5, 3);
    frame_reserve(5, 3);
    frame_reserve(6, 1);
    n = drain();
    CHECK("reserving the same frames twice is idempotent", n == 61,
          "after reserving frames 5..7 (three times over), %u frames could be taken, expected 61", n);

    frame_init();
    frame_reserve(3, 0);
    n = drain();
    CHECK("a reservation with count 0 reserves nothing", n == 64,
          "%u frames could be taken, expected 64", n);

    frame_init();
    frame_reserve(62, 1000);
    n = drain();
    CHECK("a range running past frame 63 is clamped to the pool", n == 62,
          "after frame_reserve(62, 1000), %u frames could be taken, expected 62", n);

    frame_init();
    frame_reserve(64, 5);
    frame_reserve(99, 0xFFFFFFFFu);
    n = drain();
    CHECK("a range that starts outside the pool changes nothing", n == 64,
          "%u frames could be taken, expected 64", n);

    frame_init();
    frame_reserve(10, 0xFFFFFFFFu);
    n = drain();
    CHECK("frame_reserve(10, 0xFFFFFFFF) reserves frames 10..63 despite first + count overflowing", n == 10,
          "%u frames could be taken, expected 10 (frames 0..9)", n);

    frame_init();
    frame_reserve(0, 2);
    frame_take();                       /* frame 2 */
    r = frame_release(BASE + 1);
    CHECK("frame_release rejects an unaligned address", r == -1, "frame_release(BASE + 1) returned %d, expected -1", r);
    r = frame_release(BASE - 4096);
    CHECK("frame_release rejects an address below BASE", r == -1, "frame_release(BASE - 4096) returned %d, expected -1", r);
    r = frame_release(F(64));
    CHECK("frame_release rejects the address just past the pool", r == -1, "frame_release(BASE + 64 * 4096) returned %d, expected -1", r);
    r = frame_release(0);
    CHECK("frame_release rejects address 0", r == -1, "frame_release(0) returned %d, expected -1", r);
    r = frame_release(F(9));
    CHECK("frame_release rejects a frame that is free", r == -1, "frame_release of never-taken frame 9 returned %d, expected -1", r);
    a = frame_take();
    CHECK("rejected releases leave the bitmap unchanged", a == F(3),
          "next frame_take returned 0x%lx, expected 0x%lx", a, F(3));

    r = frame_release(F(2));
    CHECK("frame_release of a taken frame returns 0", r == 0, "returned %d, expected 0", r);
    r = frame_release(F(2));
    CHECK("releasing the same frame twice returns -1", r == -1, "second release returned %d, expected -1", r);
    a = frame_take();
    CHECK("a released frame is the lowest free one again", a == F(2),
          "frame_take returned 0x%lx, expected 0x%lx", a, F(2));

    r = frame_release(F(1));
    a = frame_take();
    CHECK("a reserved frame can be released and is then handed out", r == 0 && a == F(1),
          "frame_release(frame 1) returned %d and frame_take returned 0x%lx, expected 0 and 0x%lx", r, a, F(1));

    frame_init();
    n = drain();
    a = frame_take();
    CHECK("all 64 frames can be taken, then frame_take returns 0", n == 64 && a == 0,
          "took %u frames, then frame_take returned 0x%lx", n, a);
    r = frame_release(F(63));
    a = frame_take();
    CHECK("the last frame (63) can be released and taken again", r == 0 && a == F(63),
          "frame_release returned %d, frame_take returned 0x%lx, expected 0 and 0x%lx", r, a, F(63));
}
