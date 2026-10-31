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
static unsigned long m_failed;
static long mm_alloc(size_t n, int policy) { long r = m_alloc(n, policy); if (n > 0 && r < 0) m_failed++; return r; }

static const char *check_state(int policy, int step, char *msg, size_t mn) {
    HeapStats st; size_t u, f, ub, fb, lg;
    heap_stats(&st); m_stats(&u, &f, &ub, &fb, &lg);
    if (st.search_steps != m_steps) { snprintf(msg, mn, "step %d (%s): search_steps is %lu, expected %lu", step, policy ? "best fit" : "first fit", st.search_steps, m_steps); return msg; }
    if (st.failed_allocs != m_failed) { snprintf(msg, mn, "step %d: failed_allocs is %lu, expected %lu", step, st.failed_allocs, m_failed); return msg; }
    if (st.free_blocks != fb || st.largest_free != lg || st.used_bytes != u || st.free_bytes != f) { snprintf(msg, mn, "step %d: heap shape differs (free_blocks %zu vs %zu, largest %zu vs %zu)", step, st.free_blocks, fb, st.largest_free, lg); return msg; }
    return NULL;
}

static const char *run_policy(unsigned seed, int policy, size_t len, char *msg, size_t mn) {
    unsigned s = seed;
    heap_init(hbuf, len); heap_set_policy((Policy)policy);
    m_init(len); m_failed = 0;
    unsigned char *live[200]; int nlive = 0;
    for (int step = 0; step < 400; step++) {
        if (rng(&s) % 100 < 58) {
            size_t n = 1 + rng(&s) % 90;
            if (rng(&s) % 6 == 0) n = 100 + rng(&s) % 250;
            if (rng(&s) % 40 == 0) n = 0;
            long mo = mm_alloc(n, policy);
            unsigned char *p = heap_alloc(n);
            if (mo < 0 ? p != NULL : p != hbuf + mo) { snprintf(msg, mn, "step %d (%s): heap_alloc(%zu) returned +%td, expected +%ld", step, policy ? "best fit" : "first fit", n, p ? p - hbuf : -1, mo); return msg; }
            if (p && nlive < 200) live[nlive++] = p;
        } else if (nlive > 0) {
            int i = (int)(rng(&s) % (unsigned)nlive);
            m_free(live[i] - hbuf);
            if (heap_free(live[i]) != 0) { snprintf(msg, mn, "step %d: heap_free failed", step); return msg; }
            live[i] = live[--nlive];
        }
        const char *bad = check_state(policy, step, msg, mn);
        if (bad) return bad;
    }
    msg[0] = 0; return msg;
}

static WorkResult model_workload(int policy, const Op *ops, int n) {
    m_init(2048); m_failed = 0;
    long ptr[32]; for (int i = 0; i < 32; i++) ptr[i] = -1;
    for (int i = 0; i < n; i++) {
        if (!ops[i].is_free) { if (ptr[ops[i].id] < 0) ptr[ops[i].id] = mm_alloc(ops[i].size, policy); }
        else if (ptr[ops[i].id] >= 0) { m_free(ptr[ops[i].id]); ptr[ops[i].id] = -1; }
    }
    size_t u, f, ub, fb, lg; m_stats(&u, &f, &ub, &fb, &lg);
    WorkResult r = { m_failed, m_steps, lg, fb };
    return r;
}

static int same(WorkResult a, WorkResult b) { return a.failed == b.failed && a.steps == b.steps && a.largest_free == b.largest_free && a.free_blocks == b.free_blocks; }

int main(void) {
    char msg[220];
    // The lesson's trace, with hand-derived expectations (independent of the model).
    Op trace[] = { {0,0,512}, {0,1,64}, {0,2,256}, {0,3,64}, {0,4,1072}, {1,0,0}, {1,2,0}, {0,5,200}, {0,6,300} };
    WorkResult ff = run_workload(FIRST_FIT, trace, 9), bf = run_workload(BEST_FIT, trace, 9);
    CHECK("lesson trace, first fit: the 300-byte request fails", ff.failed == 1, "failed=%lu, expected 1", ff.failed);
    CHECK("lesson trace, best fit: both requests succeed", bf.failed == 0, "failed=%lu, expected 0", bf.failed);
    CHECK("lesson trace, first fit ends with two free blocks", ff.free_blocks == 2, "free_blocks=%zu", ff.free_blocks);
    CHECK("lesson trace matches the model for both policies", same(ff, model_workload(0, trace, 9)) && same(bf, model_workload(1, trace, 9)),
          "first fit steps=%lu (model %lu), best fit steps=%lu (model %lu)", ff.steps, model_workload(0, trace, 9).steps, bf.steps, model_workload(1, trace, 9).steps);
    CHECK("best fit examines at least as many blocks as first fit here", bf.steps >= ff.steps, "best %lu < first %lu", bf.steps, ff.steps);

    heap_init(hbuf, 512); heap_set_policy(FIRST_FIT);
    HeapStats st; heap_stats(&st);
    CHECK("heap_init resets the counters", st.search_steps == 0 && st.failed_allocs == 0, "steps %lu failed %lu", st.search_steps, st.failed_allocs);
    heap_alloc(16);
    heap_stats(&st);
    CHECK("an allocation from a single free block costs one step", st.search_steps == 1, "steps %lu, expected 1", st.search_steps);
    heap_alloc(16); heap_alloc(16);
    heap_stats(&st);
    CHECK("each search walks the used blocks before the free tail", st.search_steps == 1 + 2 + 3, "steps %lu, expected 6", st.search_steps);
    heap_alloc(5000); heap_alloc(0);
    heap_stats(&st);
    CHECK("an oversized request fails without searching; zero is not a failure", st.failed_allocs == 1 && st.search_steps == 6, "failed %lu steps %lu", st.failed_allocs, st.search_steps);

    heap_init(hbuf, 512); heap_set_policy(BEST_FIT);
    unsigned char *a = heap_alloc(48), *b = heap_alloc(16), *c = heap_alloc(48), *d = heap_alloc(16);
    heap_free(a); heap_free(c);
    unsigned char *e = heap_alloc(40);
    CHECK("best fit ties go to the lowest address", e == a, "got +%td, expected +%td", e ? e - hbuf : -1, a - hbuf);
    heap_init(hbuf, 512); heap_set_policy(BEST_FIT);
    a = heap_alloc(128); b = heap_alloc(16); c = heap_alloc(32); d = heap_alloc(16);
    heap_free(a); heap_free(c);
    e = heap_alloc(30);
    CHECK("best fit prefers the smaller hole that still fits", e == c && b && d, "got +%td, expected +%td", e ? e - hbuf : -1, c - hbuf);
    heap_init(hbuf, 512); heap_set_policy(FIRST_FIT);
    a = heap_alloc(128); b = heap_alloc(16); c = heap_alloc(32); d = heap_alloc(16);
    heap_free(a); heap_free(c);
    e = heap_alloc(30);
    CHECK("first fit takes the earlier, larger hole", e == a, "got +%td, expected +%td", e ? e - hbuf : -1, a - hbuf);

    size_t lens[] = { 512, 1024, 2048, 3000 };
    for (int pol = 0; pol < 2; pol++) {
        for (unsigned k = 0; k < 4; k++) {
            char name[100];
            snprintf(name, sizeof name, "seeded random operations, %s, %zu-byte heap", pol ? "best fit" : "first fit", lens[k]);
            const char *bad = run_policy(4200u + 59u * k + 7u * pol, pol, lens[k], msg, sizeof msg);
            CHECK(name, bad[0] == 0, "%s", bad);
        }
    }

    // Random workloads through run_workload.
    int bad_count = 0; char first_bad[160] = ""; unsigned diff_seeds = 0;
    for (unsigned seed = 1; seed <= 12; seed++) {
        unsigned s = seed * 7919u;
        Op ops[150]; int n = 0; int has[32] = { 0 };
        for (int i = 0; i < 150; i++) {
            int id = (int)(rng(&s) % 32);
            if (has[id] && rng(&s) % 100 < 45) { ops[n].is_free = 1; ops[n].id = id; ops[n].size = 0; has[id] = 0; n++; }
            else if (!has[id]) { ops[n].is_free = 0; ops[n].id = id; ops[n].size = 8 + rng(&s) % 180; if (rng(&s) % 9 == 0) ops[n].size += 300; has[id] = 1; n++; }
        }
        WorkResult r0 = run_workload(FIRST_FIT, ops, n), r1 = run_workload(BEST_FIT, ops, n);
        WorkResult m0 = model_workload(0, ops, n), m1 = model_workload(1, ops, n);
        if (!same(r0, m0) || !same(r1, m1)) {
            if (!bad_count++) snprintf(first_bad, sizeof first_bad, "seed %u: first fit steps %lu/%lu failed %lu/%lu; best fit steps %lu/%lu failed %lu/%lu (yours/expected)", seed, r0.steps, m0.steps, r0.failed, m0.failed, r1.steps, m1.steps, r1.failed, m1.failed);
        }
        if (m0.steps != m1.steps || m0.failed != m1.failed) diff_seeds++;
    }
    CHECK("run_workload matches the model on 12 random workloads, both policies", bad_count == 0, "%d workloads differ. %s", bad_count, first_bad);
    CHECK("run_workload starts from a fresh heap every call", same(run_workload(FIRST_FIT, trace, 9), ff) && same(run_workload(BEST_FIT, trace, 9), bf), "results changed between identical calls");
    return 0;
}
