static _Alignas(16) unsigned char hb[8192];
static size_t hb_len;

static int in_heap(const void *p) {
    return p && (const unsigned char *)p >= hb && (const unsigned char *)p < hb + hb_len;
}
static void reset(size_t len) { hb_len = len; classes_init(hb, len); }
static unsigned rnd(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }
static size_t used_blocks(void) { HeapStats st; heap_stats(&st); return st.used_blocks; }
static int pattern_ok(const unsigned char *p, size_t n, unsigned char v) {
    for (size_t i = 0; i < n; i++) if (p[i] != v) return 0;
    return 1;
}

int main(void) {
    void *s[10], *l[10];

    // ---- Zero and NULL ----
    reset(1024);
    CHECK("classes_alloc(0) returns NULL", classes_alloc(0) == NULL, "classes_alloc(0) did not return NULL");
    CHECK("classes_free(NULL) returns 0", classes_free(NULL) == 0, "classes_free(NULL) did not return 0");
    CHECK("alloc(0) does not consume a slot or a block", used_blocks() == 0, "used_blocks %zu after alloc(0)", used_blocks());

    // ---- Small pool: lowest slot first, 16 bytes apart, outside the heap ----
    reset(1024);
    for (int i = 0; i < 8; i++) s[i] = classes_alloc(i % 2 ? 16 : 1 + (size_t)i);   // sizes 1..16 all use the small pool
    int all = 1, align = 1, outside = 1, stride = 1;
    for (int i = 0; i < 8; i++) {
        if (!s[i]) { all = 0; continue; }
        if ((uintptr_t)s[i] % 16) align = 0;
        if (in_heap(s[i])) outside = 0;
        if (i && s[i - 1] && (unsigned char *)s[i] - (unsigned char *)s[i - 1] != 16) stride = 0;
    }
    CHECK("eight small requests (1..16 bytes) all succeed", all, "a small request returned NULL");
    CHECK("small pool pointers are 16-byte aligned", all && align, "an address was not a multiple of 16");
    CHECK("small pool slots are consecutive 16-byte slots, lowest first", all && stride, "consecutive small allocations were not 16 bytes apart");
    CHECK("small pool objects are not inside the heap buffer", all && outside, "a pool object was carved out of the heap buffer");
    CHECK("pool allocations create no heap blocks", used_blocks() == 0, "used_blocks %zu, expected 0", used_blocks());

    // ---- Boundaries: 16 | 17 | 64 | 65 ----
    reset(1024);
    void *b16 = classes_alloc(16), *b17 = classes_alloc(17), *b17b = classes_alloc(64), *b65 = classes_alloc(65);
    CHECK("16 bytes goes to the 16-byte pool", b16 && !in_heap(b16), "16-byte request was not served from the small pool");
    CHECK("17 bytes goes to the 64-byte pool", b17 && b17b && !in_heap(b17) && (unsigned char *)b17b - (unsigned char *)b17 == 64,
          "17 and 64-byte requests should be consecutive 64-byte slots");
    CHECK("65 bytes goes to the heap", in_heap(b65), "65-byte request was not served from the heap");
    CHECK("only the 65-byte request made a heap block", used_blocks() == 1, "used_blocks %zu, expected 1", used_blocks());

    // ---- Small pool exhaustion: the 9th request falls back to the heap, not to the large pool ----
    reset(1024);
    for (int i = 0; i < 8; i++) s[i] = classes_alloc(8);
    void *ninth = classes_alloc(8);
    CHECK("a full small pool falls back to the heap", in_heap(ninth), "9th small request was not served from the heap");
    CHECK("fallback creates exactly one heap block", used_blocks() == 1, "used_blocks %zu, expected 1", used_blocks());
    int large_ok = 1;
    for (int i = 0; i < 8; i++) { l[i] = classes_alloc(40); if (!l[i] || in_heap(l[i])) large_ok = 0; }
    CHECK("small overflow does not use up large slots", large_ok && used_blocks() == 1, "large pool should still hold 8 slots; used_blocks %zu", used_blocks());
    void *l9 = classes_alloc(40);
    CHECK("a full large pool falls back to the heap", in_heap(l9) && used_blocks() == 2, "9th large request: in_heap=%d used_blocks=%zu", in_heap(l9), used_blocks());

    // ---- Contents of live allocations are preserved ----
    reset(4096);
    void *live[24]; size_t sz[24];
    int fine = 1;
    for (int i = 0; i < 24; i++) {
        sz[i] = i < 8 ? 16 : i < 16 ? 64 : 80 + (size_t)i;
        live[i] = classes_alloc(sz[i]);
        if (!live[i]) { fine = 0; continue; }
        memset(live[i], 0xA0 + i, sz[i]);
    }
    int intact = fine;
    for (int i = 0; i < 24 && intact; i++) if (!pattern_ok(live[i], sz[i], (unsigned char)(0xA0 + i))) intact = 0;
    CHECK("24 mixed allocations all succeed in a 4096-byte heap", fine, "an allocation returned NULL");
    CHECK("no live allocation overwrites another", intact, "a pattern written to one allocation changed");

    // ---- classes_free: pool pointers ----
    reset(1024);
    for (int i = 0; i < 8; i++) s[i] = classes_alloc(16);
    CHECK("an interior pointer into a pool slot is rejected", classes_free((unsigned char *)s[2] + 1) == -1, "interior pointer was not rejected");
    CHECK("freeing an exact slot start returns 0", classes_free(s[2]) == 0, "valid pool free failed");
    CHECK("freeing the same slot again returns -1", classes_free(s[2]) == -1, "double free was accepted");
    classes_free(s[5]);
    void *r1 = classes_alloc(16), *r2 = classes_alloc(16);
    CHECK("freed slots are reused lowest first", r1 == s[2] && r2 == s[5], "got slots %p and %p, expected %p then %p", r1, r2, s[2], s[5]);
    classes_free(s[1]); classes_free(s[1]); classes_free(s[1]);
    void *x1 = classes_alloc(16), *x2 = classes_alloc(16);
    CHECK("a rejected double free does not hand a slot out twice", x1 == s[1] && x2 != x1 && in_heap(x2), "slot %p handed out %s", x1, x1 == x2 ? "twice" : "wrongly");
    CHECK("a pointer 64 bytes past the small pool end is not a pool slot", classes_free((unsigned char *)s[7] + 16 + 64) == -1, "out-of-range pointer was accepted");

    // ---- classes_free: large pool and heap ----
    reset(1024);
    void *L0 = classes_alloc(33), *L1 = classes_alloc(64);
    CHECK("an interior pointer into a large slot is rejected", classes_free((unsigned char *)L0 + 16) == -1, "interior pointer was not rejected");
    CHECK("freeing a large slot returns 0", classes_free(L1) == 0 && classes_free(L1) == -1, "large pool free or double free wrong");
    CHECK("a freed large slot is reused", classes_alloc(20) == L1, "freed large slot was not reused");
    void *h1 = classes_alloc(100), *h2 = classes_alloc(200);
    CHECK("heap pointers free through the heap", classes_free(h1) == 0 && classes_free(h2) == 0, "heap pointer free failed");
    HeapStats st; heap_stats(&st);
    CHECK("freed heap blocks coalesce back into one block", st.used_blocks == 0 && st.free_blocks == 1, "used %zu free %zu", st.used_blocks, st.free_blocks);
    CHECK("freeing a heap pointer twice returns -1", classes_free(h1) == -1, "double free of a heap pointer was accepted");
    int local = 0;
    CHECK("a foreign pointer is rejected", classes_free(&local) == -1, "classes_free accepted a pointer it never returned");
    void *h3 = classes_alloc(96);
    CHECK("an interior heap pointer is rejected", classes_free((unsigned char *)h3 + 16) == -1, "interior heap pointer accepted");

    // ---- Heap exhaustion returns NULL; pools still work ----
    reset(256);
    int got = 0; void *hp;
    while ((hp = classes_alloc(100)) && got < 50) got++;
    CHECK("when the heap is full, a big request returns NULL", hp == NULL && got >= 1 && got < 50, "after %d allocations the heap never reported exhaustion", got);
    CHECK("pools keep working when the heap is full", classes_alloc(8) != NULL, "small request failed although the pool was empty");

    // ---- Re-initialisation resets the pools ----
    reset(1024);
    void *first = classes_alloc(1);
    for (int i = 0; i < 7; i++) classes_alloc(1);
    reset(1024);
    CHECK("classes_init makes all slots free again", classes_alloc(1) == first, "slot 0 was not available after classes_init");
    CHECK("classes_init also clears the heap", used_blocks() == 0, "used_blocks %zu after classes_init", used_blocks());

    // ---- Seeded random mix: every live block keeps its pattern, frees all succeed ----
    size_t lens[] = { 512, 2048, 8192 };
    for (unsigned k = 0; k < 3; k++) {
        reset(lens[k]);
        unsigned seed = 900u + 13u * k;
        void *p[64]; size_t n[64]; unsigned char v[64]; int cnt = 0, bad = 0, badfree = 0;
        for (int step = 0; step < 300; step++) {
            if (cnt < 64 && rnd(&seed) % 100 < 55) {
                size_t req = 1 + rnd(&seed) % 120;
                void *q = classes_alloc(req);
                if (q) {
                    if ((uintptr_t)q % 16) bad = 1;
                    p[cnt] = q; n[cnt] = req; v[cnt] = (unsigned char)(step + 1);
                    memset(q, v[cnt], req); cnt++;
                }
            } else if (cnt) {
                int i = (int)(rnd(&seed) % (unsigned)cnt);
                if (!pattern_ok(p[i], n[i], v[i])) bad = 1;
                if (classes_free(p[i]) != 0) badfree = 1;
                cnt--; p[i] = p[cnt]; n[i] = n[cnt]; v[i] = v[cnt];
            }
        }
        for (int i = 0; i < cnt; i++) { if (!pattern_ok(p[i], n[i], v[i])) bad = 1; if (classes_free(p[i]) != 0) badfree = 1; }
        char name[100];
        snprintf(name, sizeof name, "random mix in a %zu-byte heap keeps every live allocation intact", lens[k]);
        CHECK(name, !bad, "an allocation was misaligned or its contents changed");
        snprintf(name, sizeof name, "random mix in a %zu-byte heap: every valid free returns 0 and the heap empties", lens[k]);
        HeapStats fs; heap_stats(&fs);
        CHECK(name, !badfree && fs.used_blocks == 0 && fs.free_blocks == 1, "badfree=%d used_blocks=%zu free_blocks=%zu", badfree, fs.used_blocks, fs.free_blocks);
    }
    return 0;
}
