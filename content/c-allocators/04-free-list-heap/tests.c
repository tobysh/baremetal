static _Alignas(16) unsigned char hbuf[4096];
// ---- Reference model: segments in an array, no pointers. ----
typedef struct { size_t off, size; int free; } Seg;   // off = header offset
static Seg seg[512]; static int nseg; static size_t mlen;
static unsigned long m_steps;
static void m_init(size_t len) {
    mlen = len & ~(size_t)15; nseg = 0; m_steps = 0;
    if (mlen >= 32) { seg[0].off = 0; seg[0].size = mlen - 16; seg[0].free = 1; nseg = 1; }
}
// policy 0 = first fit, 1 = best fit. Returns payload offset or -1.
static long m_alloc(size_t n, int policy) {
    if (n == 0 || n > mlen) return -1;
    size_t need = (n + 15) & ~(size_t)15;
    int pick = -1;
    for (int i = 0; i < nseg; i++) {
        m_steps++;
        if (!seg[i].free || seg[i].size < need) continue;
        if (policy == 0) { pick = i; break; }
        if (pick < 0 || seg[i].size < seg[pick].size) pick = i;
    }
    if (pick < 0) return -1;
    if (seg[pick].size >= need + 32) {
        memmove(&seg[pick + 2], &seg[pick + 1], (size_t)(nseg - pick - 1) * sizeof(Seg));
        seg[pick + 1].off = seg[pick].off + 16 + need;
        seg[pick + 1].size = seg[pick].size - need - 16;
        seg[pick + 1].free = 1;
        seg[pick].size = need;
        nseg++;
    }
    seg[pick].free = 0;
    return (long)seg[pick].off + 16;
}
static int m_free(long payload) {
    for (int i = 0; i < nseg; i++) {
        if ((long)seg[i].off + 16 != payload) continue;
        if (seg[i].free) return -1;
        seg[i].free = 1;
        if (i + 1 < nseg && seg[i + 1].free) {
            seg[i].size += 16 + seg[i + 1].size;
            memmove(&seg[i + 1], &seg[i + 2], (size_t)(nseg - i - 2) * sizeof(Seg)); nseg--;
        }
        if (i > 0 && seg[i - 1].free) {
            seg[i - 1].size += 16 + seg[i].size;
            memmove(&seg[i], &seg[i + 1], (size_t)(nseg - i - 1) * sizeof(Seg)); nseg--;
        }
        return 0;
    }
    return -1;
}
static void m_stats(size_t *used, size_t *fre, size_t *ub, size_t *fb, size_t *largest) {
    *used = *fre = *ub = *fb = *largest = 0;
    for (int i = 0; i < nseg; i++) {
        if (seg[i].free) { (*fb)++; *fre += seg[i].size; if (seg[i].size > *largest) *largest = seg[i].size; }
        else { (*ub)++; *used += seg[i].size; }
    }
}
static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

typedef struct { unsigned char *p; size_t n; unsigned char tag; int id; } Live;

static int stats_match(char *msg, size_t mn, int step) {
    HeapStats st; size_t u, f, ub, fb, lg;
    heap_stats(&st); m_stats(&u, &f, &ub, &fb, &lg);
    if (st.used_bytes != u || st.free_bytes != f || st.used_blocks != ub || st.free_blocks != fb || st.largest_free != lg) {
        snprintf(msg, mn, "step %d: stats used=%zu/%zu free=%zu/%zu used_blocks=%zu/%zu free_blocks=%zu/%zu largest=%zu/%zu (yours/expected)",
                 step, st.used_bytes, u, st.free_bytes, f, st.used_blocks, ub, st.free_blocks, fb, st.largest_free, lg);
        return 0;
    }
    return 1;
}

static const char *run_seed(unsigned seed, size_t len, char *msg, size_t mn) {
    unsigned s = seed;
    size_t eff = len & ~(size_t)15;   // heap_init rounds the size down
    memset(hbuf, 0xEE, sizeof hbuf);
    heap_init(hbuf, len);
    m_init(len);
    Live live[300]; int nlive = 0;
    for (int step = 0; step < 500; step++) {
        unsigned r = rng(&s) % 100;
        if (r < 55) {
            size_t n = 1 + rng(&s) % 120;
            unsigned k = rng(&s) % 40;
            if (k == 0) n = 0;
            else if (k == 1) n = SIZE_MAX - rng(&s) % 3;
            else if (k == 2) n = eff - 16 - rng(&s) % 48;
            else if (k == 3) n = 200 + rng(&s) % 300;
            long mo = m_alloc(n, 0);
            void *p = heap_alloc(n);
            if (mo < 0) { if (p) { snprintf(msg, mn, "step %d: heap_alloc(%zu) returned %p, expected NULL", step, n, p); return msg; } }
            else {
                if (!p) { snprintf(msg, mn, "step %d: heap_alloc(%zu) returned NULL, expected payload at +%ld", step, n, mo); return msg; }
                if ((unsigned char *)p != hbuf + mo) { snprintf(msg, mn, "step %d: heap_alloc(%zu) returned +%td, expected +%ld (first fit, address order)", step, n, (unsigned char *)p - hbuf, mo); return msg; }
                if ((uintptr_t)p % 16) { snprintf(msg, mn, "step %d: %p is not 16-aligned", step, p); return msg; }
                if (nlive < 300) { live[nlive].p = p; live[nlive].n = n; live[nlive].tag = (unsigned char)(step * 13 + 5); memset(p, live[nlive].tag, n); nlive++; }
            }
        } else if (r < 92 && nlive > 0) {
            int i = (int)(rng(&s) % (unsigned)nlive);
            for (size_t k = 0; k < live[i].n; k++) if (live[i].p[k] != live[i].tag) { snprintf(msg, mn, "step %d: a live block was overwritten (blocks overlap or a header landed in a payload)", step); return msg; }
            long off = live[i].p - hbuf;
            int mr = m_free(off), rc = heap_free(live[i].p);
            if (rc != mr) { snprintf(msg, mn, "step %d: heap_free returned %d, expected %d", step, rc, mr); return msg; }
            live[i] = live[--nlive];
        } else if (nlive > 0) {
            int i = (int)(rng(&s) % (unsigned)nlive);
            unsigned char *bad = live[i].p; const char *what;
            switch (rng(&s) % 3) {
                case 0: bad = live[i].p + 1; what = "an interior pointer"; break;
                case 1: bad = live[i].p + 8; what = "a pointer into the middle of a block"; break;
                default: bad = hbuf + len + 64; what = "a pointer past the heap"; break;
            }
            if (heap_free(bad) != -1) { snprintf(msg, mn, "step %d: freeing %s did not return -1", step, what); return msg; }
        }
        if (!stats_match(msg, mn, step)) return msg;
    }
    // Free the rest in a shuffled order (Fisher-Yates), then demand complete coalescing.
    for (int i = nlive - 1; i > 0; i--) { int j = (int)(rng(&s) % (unsigned)(i + 1)); Live t = live[i]; live[i] = live[j]; live[j] = t; }
    for (int i = 0; i < nlive; i++) {
        if (heap_free(live[i].p) != 0) { snprintf(msg, mn, "final cleanup: heap_free of a live block failed"); return msg; }
        m_free(live[i].p - hbuf);
    }
    HeapStats st; heap_stats(&st);
    if (st.free_blocks != 1 || st.largest_free != eff - 16 || st.used_blocks != 0) {
        snprintf(msg, mn, "after freeing everything: %zu free blocks, largest %zu; expected 1 block of %zu (coalescing is incomplete)", st.free_blocks, st.largest_free, eff - 16);
        return msg;
    }
    void *big = heap_alloc(eff - 16);
    if (big != hbuf + 16) { snprintf(msg, mn, "after freeing everything, a request for the whole heap (%zu bytes) failed", eff - 16); return msg; }
    msg[0] = 0;
    return msg;
}

int main(void) {
    HeapStats st;
    heap_init(hbuf, 1024);
    heap_stats(&st);
    CHECK("a fresh heap is one free block of len - 16", st.free_blocks == 1 && st.largest_free == 1008 && st.used_blocks == 0, "free_blocks %zu largest %zu", st.free_blocks, st.largest_free);
    CHECK("zero-byte and oversized requests fail", heap_alloc(0) == NULL && heap_alloc(1025) == NULL && heap_alloc(SIZE_MAX) == NULL && heap_alloc(SIZE_MAX - 8) == NULL, "a bad request returned memory");
    unsigned char *a = heap_alloc(1);
    CHECK("the first payload sits right after one 16-byte header", a == hbuf + 16 && ((uintptr_t)a % 16) == 0, "returned +%td", a ? a - hbuf : -1);
    unsigned char *b = heap_alloc(17);
    CHECK("1 byte occupies 16, so the next block follows at +48", b == hbuf + 48, "returned +%td", b ? b - hbuf : -1);
    unsigned char *c = heap_alloc(16);
    CHECK("17 bytes round up to 32", c == hbuf + 96, "returned +%td, expected +96", c ? c - hbuf : -1);
    heap_stats(&st);
    CHECK("used bytes count rounded payloads, not headers", st.used_bytes == 16 + 32 + 16 && st.used_blocks == 3, "used_bytes %zu used_blocks %zu", st.used_bytes, st.used_blocks);

    heap_free(a); heap_free(c);
    heap_stats(&st);
    CHECK("freeing a and c leaves two free blocks (c merges with the tail)", st.free_blocks == 2, "free_blocks %zu, expected 2 (a, and c merged with the tail)", st.free_blocks);
    CHECK("freeing b merges both neighbours into one block", heap_free(b) == 0, "heap_free(b) failed");
    heap_stats(&st);
    CHECK("free order a, c, b coalesces to one block", st.free_blocks == 1 && st.largest_free == 1008, "free_blocks %zu largest %zu, expected 1 and 1008", st.free_blocks, st.largest_free);

    heap_init(hbuf, 160);                 // one block of 144 payload bytes
    unsigned char *x = heap_alloc(112);   // remainder 32 -> split, leaves a 16-byte free block
    heap_stats(&st);
    CHECK("a remainder of header + 16 bytes is split off", x != NULL && st.free_blocks == 1 && st.largest_free == 16, "free_blocks %zu largest %zu", st.free_blocks, st.largest_free);
    heap_init(hbuf, 160);
    x = heap_alloc(128);                  // remainder 16 < 32 -> hand out the whole block
    heap_stats(&st);
    CHECK("a remainder too small for a header plus payload is not split", x != NULL && st.free_blocks == 0 && st.used_bytes == 144, "free_blocks %zu used_bytes %zu", st.free_blocks, st.used_bytes);
    CHECK("exhausted heap returns NULL", heap_alloc(1) == NULL, "allocated from a full heap");

    heap_init(hbuf, 1024);
    int count = 0; while (heap_alloc(16)) count++;
    CHECK("overhead bound: a 1024-byte heap serves exactly 32 blocks of 16 bytes", count == 32, "served %d", count);
    heap_init(hbuf, 1024);
    unsigned char *q = heap_alloc(64);
    CHECK("bad frees return -1", heap_free(q + 1) == -1 && heap_free(q + 16) == -1 && heap_free(hbuf + 2000) == -1 && heap_free(NULL) == 0, "bad free accepted");
    CHECK("double free is detected", heap_free(q) == 0 && heap_free(q) == -1, "second free of the same block did not return -1");
    heap_stats(&st);
    CHECK("a rejected free changes nothing", st.free_blocks == 1 && st.largest_free == 1008, "free_blocks %zu", st.free_blocks);
    heap_init(hbuf, 20);
    CHECK("a heap too small for one block is empty", heap_alloc(1) == NULL, "allocated from a 20-byte heap");

    char msg[220];
    size_t lens[] = { 512, 1024, 2000, 4096, 1000, 784 };
    for (unsigned k = 0; k < 6; k++) {
        char name[90];
        snprintf(name, sizeof name, "seeded random operations on a %zu-byte heap vs the reference model", lens[k]);
        const char *bad = run_seed(7000u + 101u * k, lens[k], msg, sizeof msg);
        CHECK(name, bad[0] == 0, "%s", bad);
    }
    return 0;
}
