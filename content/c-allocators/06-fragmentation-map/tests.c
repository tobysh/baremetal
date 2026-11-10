// Build the expected map from the heap's own stats-independent description:
// a list of (used?, units) blocks. Used to compare against heap_map.
typedef struct { int used; unsigned units; } Seg;

static void expected_map(const Seg *s, int n, char *out) {
    size_t k = 0;
    for (int i = 0; i < n; i++) {
        out[k++] = 'H';
        for (unsigned u = 0; u < s[i].units; u++) out[k++] = s[i].used ? 'U' : '.';
    }
    out[k] = 0;
}

static unsigned long long fingerprint(const unsigned char *p, size_t n) {
    unsigned long long h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 1099511628211ULL;
    return h;
}

static unsigned rnd(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

int main(void) {
    _Alignas(16) static unsigned char buf[2048];
    static char out[1024], want[1024];

    // 1. Fresh heaps of several sizes are a single free block.
    size_t lens[] = { 64, 256, 512, 1000 };   // 1000 is rounded down to 992 by heap_init
    for (unsigned i = 0; i < 4; i++) {
        heap_init(buf, lens[i]);
        size_t rounded = lens[i] & ~(size_t)15;
        Seg one[] = { { 0, (unsigned)((rounded - 16) / 16) } };
        expected_map(one, 1, want);
        size_t n = heap_map(out, sizeof out);
        CHECK("fresh heap is one header followed by free units", n == strlen(want) && strcmp(out, want) == 0,
              "heap of %zu bytes: got %zu chars \"%.40s\", expected %zu chars \"%.40s\"", lens[i], n, out, strlen(want), want);
    }

    // 2. Used blocks, rounding and the split remainder, in a 256-byte heap.
    heap_init(buf, 256);
    void *a = heap_alloc(32), *b = heap_alloc(16);
    size_t n = heap_map(out, sizeof out);
    CHECK("used blocks print as U, the split remainder stays free", strcmp(out, "HUUHUH..........") == 0,
          "after alloc(32), alloc(16) got \"%s\", expected \"HUUHUH..........\"", out);
    heap_free(a);
    heap_map(out, sizeof out);
    CHECK("a freed block that cannot merge becomes a hole", strcmp(out, "H..HUH..........") == 0,
          "after free(a) got \"%s\", expected \"H..HUH..........\"", out);
    CHECK("the length is always 16 for this heap", n == 16, "returned %zu, expected 16", n);

    heap_init(buf, 256);
    void *c = heap_alloc(17);   // rounds up to 32 bytes = 2 units
    heap_map(out, sizeof out);
    CHECK("a 17-byte request shows two U units (rounding up)", strncmp(out, "HUUH", 4) == 0, "got \"%s\"", out);
    (void)c;

    // 3. Freeing merges blocks, so headers disappear from the map.
    heap_init(buf, 256);
    a = heap_alloc(32); b = heap_alloc(16); c = heap_alloc(16);
    void *d = heap_alloc(16);
    heap_free(a); heap_free(c);
    heap_map(out, sizeof out);
    CHECK("two holes separated by used blocks keep their own headers", strcmp(out, "H..HUH.HUH......") == 0,
          "got \"%s\", expected \"H..HUH.HUH......\"", out);
    heap_free(b);
    heap_map(out, sizeof out);
    CHECK("freeing the middle block merges three blocks into one hole", strcmp(out, "H......HUH......") == 0,
          "got \"%s\", expected \"H......HUH......\" (the merged hole is 6 units)", out);
    heap_free(d);
    heap_map(out, sizeof out);
    CHECK("freeing the last used block coalesces the heap back to one block", strcmp(out, "H...............") == 0,
          "got \"%s\", expected a single header and 15 dots", out);

    // 4. Buffer sizes: return value is constant, writes never pass cap - 1 + NUL.
    heap_init(buf, 256);
    a = heap_alloc(32); b = heap_alloc(16); heap_free(a);
    strcpy(want, "H..HUH..........");
    int ret_ok = 1, nul_ok = 1, prefix_ok = 1, guard_ok = 1;
    size_t bad_cap = 0;
    for (size_t cap = 1; cap <= 20; cap++) {
        char small[40];
        memset(small, 'X', sizeof small);
        size_t got = heap_map(small, cap);
        size_t len = cap - 1 < 16 ? cap - 1 : 16;
        if (got != 16) { ret_ok = 0; bad_cap = cap; }
        if (small[len] != 0) { nul_ok = 0; bad_cap = cap; }
        if (memcmp(small, want, len) != 0) { prefix_ok = 0; bad_cap = cap; }
        if (small[cap] != 'X') { guard_ok = 0; bad_cap = cap; }
    }
    CHECK("the return value is the full length for every cap", ret_ok, "wrong return value, first at cap %zu", bad_cap);
    CHECK("the output is NUL-terminated right after the last character that fits", nul_ok, "no NUL at the expected place, first at cap %zu", bad_cap);
    CHECK("a short buffer receives a correct prefix of the map", prefix_ok, "prefix differs from the full map, first at cap %zu", bad_cap);
    CHECK("nothing is written at or beyond out[cap]", guard_ok, "byte at out[cap] was overwritten, first at cap %zu", bad_cap);
    CHECK("cap 0 with a NULL buffer only measures", heap_map(NULL, 0) == 16, "heap_map(NULL, 0) did not return 16");
    char one[3] = { 'X', 'X', 'X' };
    CHECK("cap 1 writes only the terminator", heap_map(one, 1) == 16 && one[0] == 0 && one[1] == 'X', "one[0]=%d one[1]=%c", one[0], one[1]);

    // 5. Read-only: heap bytes and stats are untouched.
    HeapStats s0, s1;
    heap_stats(&s0);
    unsigned long long f0 = fingerprint(buf, 256);
    for (int i = 0; i < 5; i++) heap_map(out, sizeof out);
    heap_stats(&s1);
    CHECK("heap_map does not change heap bytes", fingerprint(buf, 256) == f0, "the heap buffer changed");
    CHECK("heap_map does not change counters", s0.search_steps == s1.search_steps && s0.failed_allocs == s1.failed_allocs
          && s0.used_blocks == s1.used_blocks && s0.free_blocks == s1.free_blocks, "steps %lu->%lu, free_blocks %zu->%zu",
          s0.search_steps, s1.search_steps, s0.free_blocks, s1.free_blocks);
    (void)b;

    // 6. Empty heaps.
    memset(out, 'X', 8);
    heap_init(buf, 0);
    CHECK("a zero-byte heap gives an empty string", heap_map(out, sizeof out) == 0 && out[0] == 0, "got length/first byte %d", out[0]);
    memset(out, 'X', 8);
    heap_init(buf, 31);
    CHECK("a heap too small for one block gives an empty string", heap_map(out, sizeof out) == 0 && out[0] == 0, "first byte %d", out[0]);
    heap_init(buf, 32);
    CHECK("the smallest heap is a header and one free unit", heap_map(out, sizeof out) == 2 && strcmp(out, "H.") == 0, "got \"%s\"", out);

    // 7. Random sequences: the map must agree with heap_stats (headers, used and free units).
    size_t sizes[] = { 256, 512, 1024, 2048 };
    for (unsigned k = 0; k < 4; k++) {
        unsigned seed = 77u + 31u * k;
        heap_init(buf, sizes[k]);
        void *live[64]; int nl = 0;
        for (int step = 0; step < 150; step++) {
            if (rnd(&seed) % 100 < 60) {
                void *p = heap_alloc(1 + rnd(&seed) % 100);
                if (p && nl < 64) live[nl++] = p;
            } else if (nl) {
                int i = (int)(rnd(&seed) % (unsigned)nl);
                heap_free(live[i]);
                live[i] = live[--nl];
            }
        }
        HeapStats st; heap_stats(&st);
        size_t need = heap_map(NULL, 0);
        size_t got = heap_map(out, sizeof out);
        size_t H = 0, U = 0, F = 0;
        for (size_t i = 0; out[i]; i++) { if (out[i] == 'H') H++; else if (out[i] == 'U') U++; else if (out[i] == '.') F++; }
        char name[100];
        snprintf(name, sizeof name, "random workload in a %zu-byte heap: map agrees with heap_stats", sizes[k]);
        CHECK(name, need == got && H == st.used_blocks + st.free_blocks && U * 16 == st.used_bytes && F * 16 == st.free_bytes,
              "map has %zu H, %zu U, %zu '.'; stats say %zu blocks, %zu used bytes, %zu free bytes",
              H, U, F, st.used_blocks + st.free_blocks, st.used_bytes, st.free_bytes);
    }
    return 0;
}
