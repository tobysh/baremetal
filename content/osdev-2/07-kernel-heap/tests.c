static long off_of(void *p) { return p ? (long)((unsigned char *)p - arena) : -1; }

void kmain(void) {
    void *a, *b, *c, *d;
    int r;

    heap_init();
    a = heap_alloc(9);
    CHECK("the first allocation starts at the beginning of the arena", a == arena,
          "heap_alloc(9) returned offset %ld, expected 0", off_of(a));
    CHECK("returned pointers are 8-byte aligned", a && ((uintptr_t)a % 8) == 0,
          "pointer 0x%lx is not 8-aligned", (unsigned long)(uintptr_t)a);
    b = heap_alloc(24);
    CHECK("9 bytes round up to 16, so the next block starts at offset 16", off_of(b) == 16,
          "heap_alloc(24) after heap_alloc(9) returned offset %ld, expected 16", off_of(b));
    c = heap_alloc(16);
    CHECK("an exact multiple of 8 is not rounded further", off_of(c) == 40,
          "heap_alloc(16) returned offset %ld, expected 40 (16 + 24)", off_of(c));

    r = heap_free((char *)b + 1);
    CHECK("heap_free rejects a pointer into the middle of a block", r == -1, "heap_free(b + 1) returned %d, expected -1", r);
    r = heap_free((char *)b + 8);
    CHECK("heap_free rejects a pointer 8 bytes into a block", r == -1, "heap_free(b + 8) returned %d, expected -1", r);
    r = heap_free(NULL);
    CHECK("heap_free(NULL) returns -1", r == -1, "returned %d", r);
    r = heap_free(arena + 256);
    CHECK("heap_free rejects a pointer just past the arena", r == -1, "heap_free(arena + 256) returned %d, expected -1", r);
    r = heap_free(b);
    CHECK("heap_free of a live block returns 0", r == 0, "heap_free(b) returned %d, expected 0", r);
    r = heap_free(b);
    CHECK("freeing the same block twice returns -1", r == -1, "second heap_free(b) returned %d, expected -1", r);

    d = heap_alloc(20);
    CHECK("first fit reuses the 24-byte hole for a 20-byte request", d == b,
          "heap_alloc(20) returned offset %ld, expected 16 (the hole)", off_of(d));
    heap_free(d);
    d = heap_alloc(8);
    CHECK("a smaller request splits the hole and takes its front", d == b,
          "heap_alloc(8) returned offset %ld, expected 16", off_of(d));
    d = heap_alloc(16);
    CHECK("the rest of the split hole is used by the next fitting request", off_of(d) == 24,
          "heap_alloc(16) returned offset %ld, expected 24 (the remaining 16 bytes of the hole)", off_of(d));
    d = heap_alloc(8);
    CHECK("when the hole is full, allocation continues after the last block", off_of(d) == 56,
          "heap_alloc(8) returned offset %ld, expected 56", off_of(d));

    heap_init();
    a = heap_alloc(80); b = heap_alloc(80); c = heap_alloc(80);
    heap_free(b); heap_free(a);
    d = heap_alloc(160);
    CHECK("a freed block merges with a free block after it", d == a,
          "freed b then a; heap_alloc(160) returned offset %ld, expected 0", off_of(d));

    heap_init();
    a = heap_alloc(80); b = heap_alloc(80); c = heap_alloc(80);
    heap_free(a); heap_free(b);
    d = heap_alloc(160);
    CHECK("a freed block merges with a free block before it", d == a,
          "freed a then b; heap_alloc(160) returned offset %ld, expected 0", off_of(d));

    heap_init();
    a = heap_alloc(80); b = heap_alloc(80); c = heap_alloc(80);
    heap_free(a); heap_free(c);
    d = heap_alloc(160);
    CHECK("free blocks that are not neighbours are not merged", d == NULL,
          "with only b busy, heap_alloc(160) returned offset %ld, expected NULL", off_of(d));
    heap_free(b);
    d = heap_alloc(256);
    CHECK("freeing the block between two free blocks merges all three (and the tail)", d == arena,
          "after freeing a, c then b, heap_alloc(256) returned offset %ld, expected 0", off_of(d));

    heap_init();
    d = heap_alloc(0);
    CHECK("heap_alloc(0) returns NULL", d == NULL, "returned offset %ld", off_of(d));
    d = heap_alloc(257);
    CHECK("a request larger than the arena returns NULL", d == NULL, "heap_alloc(257) returned offset %ld", off_of(d));
    d = heap_alloc((size_t)-1);
    CHECK("heap_alloc((size_t)-1) returns NULL rather than wrapping when rounded", d == NULL,
          "returned offset %ld", off_of(d));
    d = heap_alloc(256);
    CHECK("after failed requests the whole arena is still available", d == arena,
          "heap_alloc(256) returned offset %ld, expected 0", off_of(d));
    d = heap_alloc(1);
    CHECK("a full arena returns NULL", d == NULL, "heap_alloc(1) returned offset %ld", off_of(d));

    heap_init();
    int n = 0;
    while (heap_alloc(1) && n < 100)
        n++;
    CHECK("1-byte requests use 8 bytes each: exactly 32 fit", n == 32, "%d allocations succeeded, expected 32", n);
    r = heap_free(arena + 8 * 31);
    d = heap_alloc(8);
    CHECK("the last block can be freed and reallocated", r == 0 && off_of(d) == 248,
          "heap_free returned %d, heap_alloc(8) returned offset %ld, expected 0 and 248", r, off_of(d));
}
