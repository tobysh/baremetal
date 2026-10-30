// Checks that the cache model was not edited, using a hand-computed sequence.
static int cache_model_ok(void) {
    static _Alignas(4096) int a[4096];
    cache_reset();
    for (int i = 0; i < 16; i++) cache_touch(&a[i]);          // one line: 1 miss, 15 hits
    if (cache.misses != 1 || cache.accesses != 16) return 0;
    for (int k = 0; k < 5; k++) cache_touch(&a[k * 256]);      // 5 lines of one set (1 KiB apart); 4 ways
    if (cache.misses != 5) return 0;                           // a[0] hit, 4 new misses
    cache_touch(&a[0]);                                        // a[0] was least recently used and is gone
    if (cache.misses != 6) return 0;
    cache_touch(&a[256 * 4]);                                  // still resident
    return cache.misses == 6;
}

static void fill(unsigned seed) {
    unsigned x = seed;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++) {
            x = x * 1103515245u + 12345u;
            grid[r][c] = (int)((x >> 8) % 2001) - 1000;
        }
}

// Reference: plain loops, no cache, no learner code.
static void ref_sums(long want[N]) {
    for (int c = 0; c < N; c++) {
        long s = 0;
        for (int r = 0; r < N; r++) s += grid[r][c];
        want[c] = s;
    }
}

int main(void) {
    CHECK("the cache model has not been edited", cache_model_ok(), "the instrument gives wrong counts; restore the cache block");

    long want[N], out[N];
    int all_ok = 1;
    long worst_miss = 0, worst_acc = 0;
    for (unsigned seed = 1; seed <= 4; seed++) {
        fill(seed * 7919u);
        ref_sums(want);
        memset(out, 0x55, sizeof out);
        cache_reset();
        col_sums(out);
        if (memcmp(out, want, sizeof out) != 0) all_ok = 0;
        if (cache.misses > worst_miss) worst_miss = cache.misses;
        if (cache.accesses > worst_acc) worst_acc = cache.accesses;
    }
    CHECK("column sums match the reference on four different grids", all_ok, "some column sum differs from the reference (is out[] initialised?)");

    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++) grid[r][c] = (c == 5) ? -3 : 1;
    ref_sums(want);
    memset(out, 0x55, sizeof out);
    cache_reset();
    col_sums(out);
    CHECK("negative and constant columns are summed", out[5] == -3 * N && out[0] == N && out[63] == N, "out[5]=%ld (want %d), out[0]=%ld", out[5], -3 * N, out[0]);

    CHECK("every element is read exactly once", worst_acc == (long)N * N, "%ld accesses, expected %d: do not skip or repeat loads", worst_acc, N * N);
    CHECK("the grid is read in memory order (at most 256 misses)", worst_miss <= 256, "%ld misses, the minimum is 256; the inner loop should step along a row", worst_miss);
    return 0;
}
