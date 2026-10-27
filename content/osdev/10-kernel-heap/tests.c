#define ARENA 4096
// Guard bytes before and after the arena catch writes that stray outside it.
static uint8_t area[ARENA + 64] __attribute__((aligned(16)));
#define arena (area + 32)

static void reset(void) {
    memset(area, 0xAA, sizeof area);
    heap_init(arena, ARENA);
}
static int guards_ok(void) {
    for (int i = 0; i < 32; i++) if (area[i] != 0xAA || area[32 + ARENA + i] != 0xAA) return 0;
    return 1;
}
static int in_arena(void *p) { return (uint8_t *)p >= arena && (uint8_t *)p < arena + ARENA; }

void kmain(void) {
    reset();
    size_t full = ARENA - sizeof(Block);
    CHECK("sizeof(Block) is a multiple of 16", sizeof(Block) % 16 == 0, "it is %lu", (unsigned long)sizeof(Block));
    CHECK("a fresh heap has one free block of arena size minus a header", heap_free_bytes() == full, "free bytes %lu, expected %lu", (unsigned long)heap_free_bytes(), (unsigned long)full);

    void *a = kmalloc(1);
    CHECK("kmalloc returns a pointer inside the arena", a != NULL && in_arena(a), "got %p", a);
    CHECK("pointers are 16-byte aligned", ((uintptr_t)a & 15) == 0, "got %p", a);
    CHECK("kmalloc(1) takes 16 bytes plus a header from the free space", heap_free_bytes() == full - 16 - sizeof(Block), "free bytes %lu", (unsigned long)heap_free_bytes());
    CHECK("kmalloc(0) returns NULL", kmalloc(0) == NULL, "should be NULL");

    reset();
    void *p[5]; size_t sizes[5] = {10, 100, 33, 400, 1};
    for (int i = 0; i < 5; i++) { p[i] = kmalloc(sizes[i]); if (p[i]) memset(p[i], 0x10 + i, sizes[i]); }
    int intact = 1;
    for (int i = 0; i < 5; i++) {
        if (!p[i]) { intact = 0; break; }
        for (size_t j = 0; j < sizes[i]; j++) if (((uint8_t *)p[i])[j] != 0x10 + i) intact = 0;
    }
    CHECK("five live allocations do not overlap", intact, "an allocation was overwritten by another");
    CHECK("blocks are laid out one after another in address order", p[0] < p[1] && p[1] < p[2] && p[2] < p[3] && p[3] < p[4], "addresses are out of order");

    kfree(p[1]);
    void *again = kmalloc(100);
    CHECK("a freed block is reused by the next fitting request (first fit)", again == p[1], "got %p, expected %p", again, p[1]);

    reset();
    void *x = kmalloc(100), *y = kmalloc(100), *z = kmalloc(100);
    kfree(x); kfree(z); kfree(y);
    CHECK("freeing neighbours in any order merges them back into one block", heap_free_bytes() == full, "free bytes %lu, expected %lu", (unsigned long)heap_free_bytes(), (unsigned long)full);
    CHECK("the merged space can serve one large request", kmalloc(3900) != NULL, "kmalloc(3900) returned NULL after freeing everything");

    reset();
    void *e1 = kmalloc(1000), *e2 = kmalloc(1000), *e3 = kmalloc(1000);
    kfree(e2);
    CHECK("a hole between used blocks cannot serve a request that is too big", kmalloc(1100) != e2, "returned the too-small hole");
    (void)e1; (void)e3;

    reset();
    CHECK("a request larger than the heap returns NULL", kmalloc(ARENA) == NULL && kmalloc(1 << 20) == NULL && kmalloc((size_t)-1) == NULL, "should be NULL");
    void *big = kmalloc(full);
    CHECK("the whole payload can be allocated at once", big != NULL && heap_free_bytes() == 0, "got %p, free bytes %lu", big, (unsigned long)heap_free_bytes());
    CHECK("then there is nothing left", kmalloc(1) == NULL, "should be NULL");
    kfree(big);
    CHECK("freeing it gives everything back", heap_free_bytes() == full, "free bytes %lu", (unsigned long)heap_free_bytes());

    reset();
    void *nearly = kmalloc(full - 16);
    CHECK("no split when the leftover cannot hold a header and 16 bytes", nearly != NULL && heap_free_bytes() == 0, "free bytes %lu (the whole block should have been given out)", (unsigned long)heap_free_bytes());

    kfree(NULL);
    CHECK("kfree(NULL) does nothing", heap_free_bytes() == 0, "free bytes changed");

    reset();
    int n = 0; void *all[200];
    while (n < 200 && (all[n] = kmalloc(24)) != NULL) n++;
    CHECK("small allocations fill the arena without overlapping", n >= 60 && n < 200, "made %d allocations", n);
    for (int i = 0; i < n; i++) kfree(all[i]);
    CHECK("freeing them all restores the heap exactly", heap_free_bytes() == full, "free bytes %lu, expected %lu", (unsigned long)heap_free_bytes(), (unsigned long)full);

    reset();
    uint8_t *c = kcalloc(10, 8);
    int zero = c != NULL;
    for (int i = 0; zero && i < 80; i++) if (c[i]) zero = 0;
    CHECK("kcalloc returns zeroed memory even on dirty memory", zero, "memory was not zeroed");
    CHECK("kcalloc detects multiplication overflow", kcalloc((size_t)1 << 62, 4) == NULL && kcalloc((size_t)-1, 2) == NULL, "overflow was not detected");
    CHECK("kcalloc(0, n) and kcalloc(n, 0) return NULL", kcalloc(0, 8) == NULL && kcalloc(8, 0) == NULL, "should be NULL");

    memset(area, 0xAA, sizeof area);
    heap_init(arena + 3, 1000);
    void *u = kmalloc(8);
    CHECK("an unaligned arena start is rounded up", u != NULL && ((uintptr_t)u & 15) == 0 && (uint8_t *)u >= arena + 3 + sizeof(Block), "got %p", u);
    CHECK("an unaligned arena never gives out more than it has", heap_free_bytes() <= 1000 - sizeof(Block) - 16, "free bytes %lu", (unsigned long)heap_free_bytes());

    heap_init(arena, 20);
    CHECK("an arena too small for a block is an empty heap", kmalloc(1) == NULL && heap_free_bytes() == 0, "should be empty");

    reset();
    for (int i = 0; i < 30; i++) { void *q = kmalloc(1 + i * 7); if (q) memset(q, 0xFF, 1 + i * 7); }
    CHECK("writes to every payload never touch the bytes outside the arena", guards_ok(), "a guard byte was changed");
}
