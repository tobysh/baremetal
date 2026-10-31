static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static _Alignas(16) unsigned char pbuf[4096];

// Model: a LIFO stack of free slot indices, plus a live flag per slot.
typedef struct { int stack[256]; int top; int live[256]; int count; } PM;
static void pm_init(PM *m, int count) {
    m->count = count; m->top = 0;
    for (int i = count - 1; i >= 0; i--) m->stack[m->top++] = i;
    memset(m->live, 0, sizeof m->live);
}

static const char *run_seed(unsigned seed, size_t objsize, size_t cap, size_t exp_slot, char *msg, size_t n) {
    unsigned s = seed;
    Pool p;
    int count = (int)(cap / exp_slot);
    int got = pool_init(&p, pbuf, cap, objsize);
    if (got != count) { snprintf(msg, n, "pool_init returned %d, expected %d slots of %zu bytes", got, count, exp_slot); return msg; }
    PM m; pm_init(&m, count);
    void *ptr[256] = { 0 };
    for (int step = 0; step < 600; step++) {
        unsigned r = rng(&s) % 100;
        if (r < 50) {
            void *x = pool_alloc(&p);
            if (m.top == 0) { if (x) { snprintf(msg, n, "step %d: alloc returned %p from an exhausted pool", step, x); return msg; } continue; }
            int idx = m.stack[--m.top];
            unsigned char *want = pbuf + (size_t)idx * exp_slot;
            if (x != want) { snprintf(msg, n, "step %d: alloc returned slot at +%td, expected +%zu (slot %d)", step, x ? (unsigned char *)x - pbuf : -1, (size_t)idx * exp_slot, idx); return msg; }
            m.live[idx] = 1; ptr[idx] = x;
            memset(x, 0x40 + idx % 64, objsize);
        } else if (r < 90) {
            int idx = (int)(rng(&s) % (unsigned)count);
            if (!m.live[idx]) continue;
            for (size_t k = 0; k < objsize; k++) {
                if (((unsigned char *)ptr[idx])[k] != (unsigned char)(0x40 + idx % 64)) { snprintf(msg, n, "step %d: slot %d was overwritten while live (overlap or free list written into live data)", step, idx); return msg; }
            }
            if (pool_free(&p, ptr[idx]) != 0) { snprintf(msg, n, "step %d: freeing live slot %d returned non-zero", step, idx); return msg; }
            m.live[idx] = 0; m.stack[m.top++] = idx;
        } else {
            // misuse: must return -1 and change nothing
            size_t before = pool_available(&p);
            void *bad;
            int kind = (int)(rng(&s) % 5);
            int idx = (int)(rng(&s) % (unsigned)count);
            const char *what;
            if (kind == 0) { bad = NULL; what = "NULL"; }
            else if (kind == 1) { bad = pbuf + (size_t)count * exp_slot + 8; what = "a pointer past the pool"; }
            else if (kind == 2) { bad = pbuf + (size_t)idx * exp_slot + 1; what = "an interior pointer"; }
            else if (kind == 3) { bad = (m.live[idx]) ? NULL : pbuf + (size_t)idx * exp_slot; what = "a double free"; if (!bad) continue; }
            else { bad = pbuf - 8; what = "a pointer before the pool"; }
            int rc = pool_free(&p, bad);
            if (rc != -1) { snprintf(msg, n, "step %d: freeing %s returned %d, expected -1", step, what, rc); return msg; }
            if (pool_available(&p) != before) { snprintf(msg, n, "step %d: freeing %s changed the free count", step, what); return msg; }
        }
        if (pool_available(&p) != (size_t)m.top) { snprintf(msg, n, "step %d: pool_available is %zu, expected %d", step, pool_available(&p), m.top); return msg; }
    }
    msg[0] = 0;
    return msg;
}

int main(void) {
    Pool p;
    CHECK("pool_init counts 24-byte slots in 100 bytes", pool_init(&p, pbuf, 100, 20) == 4 && pool_available(&p) == 4, "count %zu", pool_available(&p));
    CHECK("tiny objects still get 8-byte slots", pool_init(&p, pbuf, 64, 1) == 8, "got a different slot count");
    CHECK("an exact multiple of the slot size uses every byte", pool_init(&p, pbuf, 96, 16) == 6, "expected 6 slots");
    CHECK("objsize 0 gives an empty pool", pool_init(&p, pbuf, 64, 0) == 0 && pool_alloc(&p) == NULL && pool_available(&p) == 0, "pool is not empty");
    CHECK("a buffer too small for one slot gives an empty pool", pool_init(&p, pbuf, 15, 16) == 0 && pool_alloc(&p) == NULL, "pool is not empty");
    CHECK("an enormous object size does not overflow", pool_init(&p, pbuf, 64, SIZE_MAX) == 0 && pool_init(&p, pbuf, 64, SIZE_MAX - 3) == 0, "pool_init accepted SIZE_MAX");
    CHECK("freeing into an empty pool is rejected", pool_free(&p, pbuf) == -1, "free on an empty pool did not return -1");

    pool_init(&p, pbuf, 100, 20);
    unsigned char *a = pool_alloc(&p), *b = pool_alloc(&p), *c = pool_alloc(&p), *d = pool_alloc(&p);
    CHECK("slots come out in ascending order", a == pbuf && b == pbuf + 24 && c == pbuf + 48 && d == pbuf + 72, "got +%td +%td +%td +%td", a - pbuf, b - pbuf, c - pbuf, d - pbuf);
    CHECK("an exhausted pool returns NULL", pool_alloc(&p) == NULL && pool_available(&p) == 0, "alloc did not fail");
    pool_free(&p, b); pool_free(&p, c);
    CHECK("the most recently freed slot is reused first", pool_alloc(&p) == c && pool_alloc(&p) == b, "reuse order is not LIFO");
    CHECK("freeing a pointer in the unused tail is rejected", pool_free(&p, pbuf + 96) == -1, "tail bytes are not a slot");

    char msg[200];
    struct { size_t obj, cap, slot; const char *name; } cfg[] = {
        { 1, 200, 8, "seeded random operations, 1-byte objects (8-byte slots)" },
        { 24, 1000, 24, "seeded random operations, 24-byte objects" },
        { 20, 1000, 24, "seeded random operations, 20-byte objects (padding)" },
        { 64, 4096, 64, "seeded random operations, 64-byte objects" },
        { 9, 1500, 16, "seeded random operations, 9-byte objects" },
    };
    for (int k = 0; k < 5; k++) {
        const char *bad = run_seed(555u + 31u * k, cfg[k].obj, cfg[k].cap, cfg[k].slot, msg, sizeof msg);
        CHECK(cfg[k].name, bad[0] == 0, "%s", bad);
    }
    return 0;
}
