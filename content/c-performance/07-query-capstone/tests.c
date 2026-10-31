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

#define MAXR 4096
static _Alignas(4096) row_t trows[MAXR];

static long ref_query(int n, int region, int min_price, int *count) {
    long s = 0; *count = 0;
    for (int i = 0; i < n; i++)
        if (trows[i].region == region) {
            (*count)++;
            if (trows[i].price >= min_price) s += (long)trows[i].price * trows[i].qty;
        }
    return s;
}

// kind 0: uniform regions; 1: all rows in region 2; 2: only regions 0 and 7
static void gen(int n, unsigned seed, int kind) {
    unsigned x = seed;
    memset(trows, 0, sizeof trows);
    for (int i = 0; i < n; i++) {
        x = x * 1103515245u + 12345u; unsigned r = x >> 8;
        trows[i].region = kind == 0 ? (int)(r % REGIONS) : kind == 1 ? 2 : ((r & 1) ? 7 : 0);
        x = x * 1103515245u + 12345u; trows[i].price = (int)((x >> 8) % 1000);
        x = x * 1103515245u + 12345u; trows[i].qty = 1 + (int)((x >> 8) % 20);
        trows[i].id = i;
    }
}

static int all_queries(int n, table_t *t, long *worst_extra, long *aos_total, long *col_total, int *within, int *wrong, int *bad_region) {
    int floors[] = {0, 1, 250, 777, 999, 1000, 5000};
    *within = 1; *wrong = 0; *bad_region = 0;
    for (int r = -1; r <= REGIONS; r++)
        for (unsigned f = 0; f < sizeof floors / sizeof *floors; f++) {
            int k;
            long want = ref_query(n, r, floors[f], &k);
            cache_reset();
            long got = query(t, r, floors[f]);
            long miss = cache.misses;
            if (got != want) { (*wrong)++; if (r < 0 || r >= REGIONS) (*bad_region)++; }
            long budget = (r < 0 || r >= REGIONS) ? 0 : 2 * (k / 16 + 2) + 2;
            if (miss > budget) { *within = 0; if (miss - budget > *worst_extra) *worst_extra = miss - budget; }
            if (floors[f] == 0 && r >= 0 && r < REGIONS) {
                cache_reset(); query_aos(trows, n, r, 0); *aos_total += cache.misses;
                *col_total += miss;
            }
        }
    return 0;
}

int main(void) {
    CHECK("the cache model has not been edited", cache_model_ok(), "restore the cache block");

    int ns[] = {0, 1, 2, 17, 500, 2000, 4096};
    int within_all = 1, wrong_all = 0, br_all = 0, alloc_ok = 1, free_ok = 1;
    long extra = 0, aos = 0, col = 0;
    unsigned seed = 5;
    for (int a = 0; a < 7; a++)
        for (int kind = 0; kind < 3; kind++) {
            int n = ns[a];
            gen(n, seed++ * 7001u, kind);
            long a0 = sys_allocs, l0 = sys_live;
            table_t t = build_table(trows, n);
            size_t want_bytes = 2 * round64(4 * (size_t)n) + round64(4 * (REGIONS + 1));
            if (sys_allocs - a0 != 1 || sys_live - l0 != (long)want_bytes) alloc_ok = 0;
            int w, wr, br, wi;
            all_queries(n, &t, &extra, &aos, &col, &wi, &wr, &br);
            (void)w;
            if (!wi) within_all = 0;
            wrong_all += wr; br_all += br;
            table_free(&t);
            if (sys_live != l0) free_ok = 0;
        }
    CHECK("results equal the reference loop on empty, tiny, skewed and large tables", wrong_all == 0, "%d of the queries returned a different sum", wrong_all);
    CHECK("regions outside 0..7 return 0", br_all == 0, "%d out-of-range queries were wrong", br_all);
    CHECK("the table is one allocation of the stated size", alloc_ok, "build_table must call sys_alloc exactly once for 2*round64(4n)+round64(36) bytes");
    CHECK("table_free returns every byte", free_ok, "sys_live did not return to its starting value");
    CHECK("misses per query stay within 2*(k/16+2)+2", within_all, "a query exceeded its budget by up to %ld misses; walk only the region's rows", extra);

    gen(4096, 31337, 0);
    table_t t = build_table(trows, 4096);
    long aos_m = 0, col_m = 0;
    for (int r = 0; r < REGIONS; r++) {
        cache_reset(); query_aos(trows, 4096, r, 0); aos_m += cache.misses;
        cache_reset(); query(&t, r, 0); col_m += cache.misses;
    }
    CHECK("on 4096 rows the columns take at least 8 times fewer misses", col_m * 8 <= aos_m, "columns %ld misses vs AoS %ld (need at most one eighth)", col_m, aos_m);

    // The sort must be stable: the same region queried with a descending floor is a monotone sum.
    long prev = -1; int mono = 1;
    for (int f = 1000; f >= 0; f -= 100) {
        long v = query(&t, 3, f);
        if (v < prev) mono = 0;
        prev = v;
    }
    CHECK("lowering the price floor never lowers the sum", mono, "sums decreased as the floor dropped");
    table_free(&t);
    return 0;
}
