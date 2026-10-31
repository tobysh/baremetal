// Independent model of the policy in the lesson.
typedef struct { size_t cap, used, first_cap; int chunks; } amodel;
static void am_alloc(amodel *m, size_t n, size_t align) {
    size_t off = (m->used + align - 1) & ~(align - 1);
    if (m->chunks > 0 && off + n <= m->cap) { m->used = off + n; return; }
    m->cap = n > CHUNK ? n : CHUNK;
    if (m->chunks == 0) m->first_cap = m->cap;
    m->chunks++;
    m->used = n;
}

#define MAXB 1200
static unsigned char *blk[MAXB];
static size_t blksz[MAXB];

static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 8; }

// Allocates `count` blocks with random sizes up to maxsz; returns number of problems found.
static int run_seq(arena_t *a, amodel *m, unsigned seed, int count, size_t maxsz, int *misaligned) {
    unsigned s = seed;
    *misaligned = 0;
    for (int i = 0; i < count; i++) {
        size_t n = 1 + rng(&s) % maxsz;
        size_t al = (size_t)1 << (rng(&s) % 7);
        am_alloc(m, n, al);
        unsigned char *p = arena_alloc(a, n, al);
        blk[i] = p; blksz[i] = n;
        if (!p) return -1;
        if ((uintptr_t)p % al) (*misaligned)++;
        memset(p, (i % 251) + 1, n);
    }
    int bad = 0;
    for (int i = 0; i < count; i++)
        for (size_t k = 0; k < blksz[i]; k++)
            if (blk[i][k] != (i % 251) + 1) { bad++; break; }
    return bad;
}

int main(void) {
    arena_t a; amodel m; int mis;

    memset(&a, 0, sizeof a); memset(&m, 0, sizeof m);
    sys_allocs = sys_frees = sys_live = 0;
    int bad = run_seq(&a, &m, 17, 800, 200, &mis);
    CHECK("blocks never overlap (patterns survive)", bad == 0, "%d blocks were overwritten by later allocations", bad);
    CHECK("every block is aligned as requested", mis == 0, "%d blocks were misaligned", mis);
    CHECK("system allocations match the policy (small blocks)", sys_allocs == m.chunks, "%ld sys_alloc calls, the policy needs %d", sys_allocs, m.chunks);
    arena_destroy(&a);
    CHECK("destroy frees every chunk", sys_live == 0 && sys_frees == sys_allocs, "live=%ld frees=%ld allocs=%ld", sys_live, sys_frees, sys_allocs);

    memset(&a, 0, sizeof a); memset(&m, 0, sizeof m);
    sys_allocs = sys_frees = sys_live = 0;
    bad = run_seq(&a, &m, 99, 300, 3000, &mis);
    CHECK("mixed sizes, some close to a chunk: no overlap", bad == 0 && mis == 0, "overlaps=%d misaligned=%d", bad, mis);
    CHECK("mixed sizes: system allocations match the policy", sys_allocs == m.chunks, "%ld sys_alloc calls, the policy needs %d", sys_allocs, m.chunks);
    arena_destroy(&a);

    memset(&a, 0, sizeof a); memset(&m, 0, sizeof m);
    sys_allocs = sys_frees = sys_live = 0;
    unsigned char *big = arena_alloc(&a, 10000, 64);
    am_alloc(&m, 10000, 64);
    CHECK("a request larger than a chunk gets its own chunk", big && (uintptr_t)big % 64 == 0 && sys_live == 10000, "ptr=%p live=%ld, expected a 10000-byte chunk", (void *)big, sys_live);
    if (big) memset(big, 0xEE, 10000);
    unsigned char *small = arena_alloc(&a, 100, 8);
    CHECK("an exactly-sized big chunk has no tail, so the next block starts a new chunk", small && sys_allocs == 2 && sys_live == 10000 + CHUNK, "allocs=%ld live=%ld, expected 2 and %d", sys_allocs, sys_live, 10000 + CHUNK);
    arena_destroy(&a);

    memset(&a, 0, sizeof a); memset(&m, 0, sizeof m);
    sys_allocs = sys_frees = sys_live = 0;
    run_seq(&a, &m, 5, 400, 150, &mis);
    long chunks_before = sys_allocs;
    arena_reset(&a);
    CHECK("reset keeps only the first chunk", sys_live == (long)m.first_cap && sys_frees == chunks_before - 1, "live=%ld (want %zu) frees=%ld (want %ld)", sys_live, m.first_cap, sys_frees, chunks_before - 1);
    unsigned char *p0 = arena_alloc(&a, 64, 64);
    CHECK("after reset allocation restarts at the first chunk without a system call", p0 == (unsigned char *)a.base[0] && sys_allocs == chunks_before, "ptr offset or an extra sys_alloc (allocs=%ld, before=%ld)", sys_allocs, chunks_before);
    arena_reset(&a);
    arena_reset(&a);
    CHECK("reset twice is harmless", a.nchunks == 1 && sys_live == (long)m.first_cap, "nchunks=%d", a.nchunks);
    arena_destroy(&a);
    CHECK("destroy after reset leaves nothing live", sys_live == 0 && sys_frees == sys_allocs, "live=%ld", sys_live);

    // Failure schedule: the system refuses once, at the first chunk and again at the second.
    memset(&a, 0, sizeof a);
    sys_allocs = sys_frees = sys_live = 0;
    sys_fail_next = 1;
    void *r = arena_alloc(&a, 32, 8);
    CHECK("a failing first allocation returns NULL and leaves the arena empty", r == NULL && a.nchunks == 0 && sys_allocs == 0, "r=%p nchunks=%d", r, a.nchunks);
    r = arena_alloc(&a, 3000, 8);
    CHECK("the arena works after a failure", r != NULL && a.nchunks == 1, "r=%p nchunks=%d", r, a.nchunks);
    size_t used_before = a.used;
    sys_fail_next = 1;
    void *r2 = arena_alloc(&a, 2000, 8);
    CHECK("a failure for the second chunk changes nothing", r2 == NULL && a.nchunks == 1 && a.used == used_before && sys_allocs == 1, "r2=%p nchunks=%d used %zu->%zu", r2, a.nchunks, used_before, a.used);
    void *r3 = arena_alloc(&a, 500, 8);
    CHECK("requests that still fit are served without the system", r3 != NULL && sys_allocs == 1, "allocs=%ld", sys_allocs);
    arena_destroy(&a);

    // MAXCHUNKS cap: huge requests each take a chunk.
    memset(&a, 0, sizeof a);
    sys_allocs = sys_frees = sys_live = 0;
    int got = 0;
    for (int i = 0; i < MAXCHUNKS + 3; i++) if (arena_alloc(&a, CHUNK, 1)) got++;
    CHECK("the chunk table is bounded without crashing", got == MAXCHUNKS && a.nchunks == MAXCHUNKS, "served %d, nchunks=%d", got, a.nchunks);
    arena_destroy(&a);
    return 0;
}
