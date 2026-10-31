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

// Independent ground truth: own array, own loop, the simulated cache.
static _Alignas(4096) int truth[1024];
static long truth_misses(long n, long stride) {
    cache_reset();
    for (long i = 0; i < n; i += stride) cache_touch(&truth[i]);
    return cache.misses;
}

int main(void) {
    CHECK("the cache model has not been edited", cache_model_ok(), "restore the cache block");

    long ns[] = {0, 1, 2, 15, 16, 17, 100, 255, 256, 257, 640, 1000, 1024};
    long ss[] = {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 64, 100, 500, 1024, 2000};
    int bad = 0, total = 0;
    long bn = 0, bs = 0, bw = 0, bg = 0;
    int small_bad = 0, big_bad = 0;
    for (unsigned a = 0; a < sizeof ns / sizeof *ns; a++)
        for (unsigned b = 0; b < sizeof ss / sizeof *ss; b++) {
            long want = truth_misses(ns[a], ss[b]);
            long got = predict_misses(ns[a], ss[b]);
            total++;
            if (got != want) {
                if (!bad) { bn = ns[a]; bs = ss[b]; bw = want; bg = got; }
                bad++;
                if (ss[b] < 16) small_bad++; else big_bad++;
            }
        }
    CHECK("strides of one line or more: one miss per read", big_bad == 0, "%d wrong; first n=%ld stride=%ld wanted %ld got %ld", big_bad, bn, bs, bw, bg);
    CHECK("strides below one line: count the distinct lines", small_bad == 0, "%d wrong; first n=%ld stride=%ld wanted %ld got %ld", small_bad, bn, bs, bw, bg);
    CHECK("an empty scan has no misses", predict_misses(0, 1) == 0 && predict_misses(0, 40) == 0, "got %ld", predict_misses(0, 1));
    CHECK("a stride longer than the array reads once", predict_misses(10, 5000) == 1, "got %ld, expected 1", predict_misses(10, 5000));

    // Sets touched: enumerate the sets visited by 64 steps and count the distinct ones.
    int sbad = 0; long first = 0;
    for (long s = 1; s <= 80; s++) {
        int seen[SETS] = {0}, cnt = 0;
        for (long k = 0; k < 64; k++) {
            int set = (int)((k * s) % SETS);
            if (!seen[set]) { seen[set] = 1; cnt++; }
        }
        if (sets_touched(s) != cnt) { if (!sbad) first = s; sbad++; }
    }
    CHECK("sets touched matches an enumeration for strides 1 to 80", sbad == 0, "%d wrong; first stride %ld gives %d, expected a different count", sbad, first, sets_touched(first));
    CHECK("a stride of 16 lines always lands in one set", sets_touched(16) == 1 && sets_touched(32) == 1, "got %d", sets_touched(16));
    return 0;
}
