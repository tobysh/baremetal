// ---------- independent reference and cost estimates ----------
static int ref[MAXN * 2], rn;
static int ref_insert(int k) {
    int i = 0; while (i < rn && ref[i] < k) i++;
    if (i < rn && ref[i] == k) return 0;
    for (int j = rn; j > i; j--) ref[j] = ref[j - 1];
    ref[i] = k; rn++; return 1;
}
static int ref_has(int k) { for (int i = 0; i < rn; i++) if (ref[i] == k) return 1; return 0; }
static long ref_range(int lo, int hi) { long c = 0; for (int i = 0; i < rn; i++) if (ref[i] >= lo && ref[i] <= hi) c++; return c; }
static int greater_than(int k) { int c = 0; for (int i = 0; i < rn; i++) if (ref[i] > k) c++; return c; }
static int lg1(int n) { int k = 0; while ((1 << k) < n + 1) k++; return k; }

static unsigned rs;
static unsigned rnd(void) { rs = rs * 1103515245u + 12345u; return rs >> 8; }

static op_t ops[6000];

// Builds a shuffled workload and the analytic cost estimates for both designs.
static int build(int ins, int has, int rng, double *ea, double *eb, int keyspace) {
    int n = 0;
    for (int i = 0; i < ins; i++) { ops[n].kind = OP_INS; ops[n].a = 1 + (int)(rnd() % (unsigned)keyspace); n++; }
    for (int i = 0; i < has; i++) { ops[n].kind = OP_HAS; ops[n].a = 1 + (int)(rnd() % (unsigned)keyspace); n++; }
    for (int i = 0; i < rng; i++) { ops[n].kind = OP_RANGE; ops[n].a = 1 + (int)(rnd() % (unsigned)keyspace); ops[n].b = ops[n].a + (int)(rnd() % 1000); n++; }
    for (int i = n - 1; i > 0; i--) { int j = (int)(rnd() % (unsigned)(i + 1)); op_t t = ops[i]; ops[i] = ops[j]; ops[j] = t; }
    // inserts must stay under MAXN distinct keys: the caller keeps ins <= MAXN
    rn = 0;
    *ea = 0; *eb = SLOTS;
    for (int i = 0; i < n; i++) {
        if (ops[i].kind == OP_INS) { *ea += lg1(rn) + greater_than(ops[i].a); *eb += 1.5; ref_insert(ops[i].a); }
        else if (ops[i].kind == OP_HAS) { *ea += lg1(rn); *eb += 1.5; }
        else { *ea += 2 * lg1(rn); *eb += SLOTS; }
    }
    return n;
}

static sorted_set sa;
static hash_set hb;

int main(void) {
    // ---- equal results and honest counters ----
    int bad_res = 0, bad_moves = 0, bad_sprobe = 0, bad_hprobe = 0, bad_range_b = 0, bad_dup = 0;
    long hp_total = 0, hp_ops = 0;
    for (unsigned seed = 1; seed <= 4; seed++) {
        rs = seed * 2654435u; rn = 0;
        sa_init(&sa); hb_init(&hb);
        int space = seed == 1 ? 60 : (seed == 2 ? 5000 : 100000);
        for (int step = 0; step < 1500; step++) {
            unsigned r = rnd() % 10;
            int k = 1 + (int)(rnd() % (unsigned)space);
            if (r < 5 && rn < 1500) {
                int g = greater_than(k), dup = ref_has(k);
                long m0 = sa.c.moves, p0 = sa.c.probes, q0 = hb.c.probes;
                int n_before = sa.n;
                int e = ref_insert(k);
                int x = sa_insert(&sa, k), y = hb_insert(&hb, k);
                if (x != e || y != e) bad_res++;
                if (sa.c.moves - m0 != (dup ? 0 : g)) bad_moves++;
                if (dup && sa.c.moves != m0) bad_dup++;
                if ((n_before > 0 && sa.c.probes - p0 < 1) || sa.c.probes - p0 > lg1(sa.n) + 1) bad_sprobe++;
                if (hb.c.probes - q0 < 1) bad_hprobe++;
                hp_total += hb.c.probes - q0; hp_ops++;
            } else if (r < 8) {
                long p0 = sa.c.probes, q0 = hb.c.probes;
                int e = ref_has(k);
                int x = sa_has(&sa, k), y = hb_has(&hb, k);
                if (x != e || y != e) bad_res++;
                if (rn > 0 && (sa.c.probes - p0 < 1 || sa.c.probes - p0 > lg1(sa.n) + 1)) bad_sprobe++;
                if (hb.c.probes - q0 < 1) bad_hprobe++;
                hp_total += hb.c.probes - q0; hp_ops++;
            } else {
                int hi = k + (int)(rnd() % 400);
                long p0 = sa.c.probes, q0 = hb.c.probes, m0 = sa.c.moves;
                long e = ref_range(k, hi);
                long x = sa_range(&sa, k, hi), y = hb_range(&hb, k, hi);
                if (x != e || y != e) bad_res++;
                if (hb.c.probes - q0 != SLOTS) bad_range_b++;
                if (sa.c.probes - p0 > 2 * (lg1(sa.n) + 1) || sa.c.moves != m0) bad_sprobe++;
            }
        }
    }
    CHECK("both designs agree with the reference on inserts, lookups and ranges", bad_res == 0, "%d answers differ from the reference", bad_res);
    CHECK("A: a duplicate insert moves nothing", bad_dup == 0, "%d duplicate inserts shifted keys", bad_dup);
    CHECK("A: moves equal the number of larger keys", bad_moves == 0, "%d inserts counted the wrong number of moves", bad_moves);
    CHECK("A: binary searches stay logarithmic and are counted", bad_sprobe == 0, "%d operations had too many or no probes", bad_sprobe);
    CHECK("B: every operation counts at least one probe", bad_hprobe == 0, "%d operations counted no probe", bad_hprobe);
    CHECK("B: a range scan costs exactly one probe per slot", bad_range_b == 0, "%d range queries did not cost %d probes", bad_range_b, SLOTS);
    CHECK("B: lookups and inserts average under 3 probes", hp_ops > 0 && hp_total < 3 * hp_ops, "%ld probes over %ld operations", hp_total, hp_ops);
    {
        sorted_set s; hash_set h;
        sa_init(&s); hb_init(&h);
        CHECK("initialisation costs: A none, B one write per slot", s.c.init == 0 && h.c.init == SLOTS, "A.init=%ld B.init=%ld", s.c.init, h.c.init);
    }
    {
        sorted_set s; sa_init(&s);
        for (int k = 1; k <= 50; k++) sa_insert(&s, k);
        CHECK("A: inserting in ascending order moves nothing", s.c.moves == 0 && s.n == 50, "moves=%ld n=%d", s.c.moves, s.n);
        CHECK("A: an inverted range is empty", sa_range(&s, 20, 10) == 0, "got %ld", sa_range(&s, 20, 10));
        CHECK("A: range endpoints are inclusive", sa_range(&s, 10, 20) == 11 && sa_range(&s, 0, 1) == 1 && sa_range(&s, 50, 99) == 1, "got %ld %ld %ld", sa_range(&s, 10, 20), sa_range(&s, 0, 1), sa_range(&s, 50, 99));
    }

    // ---- the chooser against an independent estimate ----
    int shapes[][4] = {      // ins, has, ranges, keyspace
        {1500, 4000, 0, 100000}, {1000, 3000, 5, 100000}, {200, 0, 3000, 100000},
        {300, 20, 2500, 5000}, {40, 40, 0, 1000}, {5, 5, 0, 100}, {2000, 100, 0, 1000000},
        {100, 3000, 20, 100000}, {30, 0, 2, 1000}, {500, 5000, 1, 100000}, {10, 0, 40, 500},
    };
    int checked = 0, wrong = 0, wi = -1;
    char wrong_got = 0, wrong_exp = 0;
    for (unsigned s = 0; s < sizeof shapes / sizeof *shapes; s++) {
        rs = 1000 + s * 31;
        double ea, eb;
        int n = build(shapes[s][0], shapes[s][1], shapes[s][2], &ea, &eb, shapes[s][3]);
        char exp;
        if (ea * 3 <= eb) exp = 'A'; else if (eb * 3 <= ea) exp = 'B'; else continue;   // skip unclear cases
        char got = choose_design(ops, n, LONG_MAX);
        checked++;
        if (got != exp) { if (wi < 0) { wi = (int)s; wrong_got = got; wrong_exp = exp; } wrong++; }
    }
    CHECK("the chooser agrees with the analytic estimate on clear workloads", checked >= 8 && wrong == 0, "%d of %d clear workloads wrong; shape %d gave '%c', expected '%c'", wrong, checked, wi, wrong_got ? wrong_got : '-', wrong_exp ? wrong_exp : '-');

    rs = 4242;
    double ea, eb;
    int n = build(500, 2000, 0, &ea, &eb, 100000);
    CHECK("lookup-heavy workload with room: hash table", choose_design(ops, n, LONG_MAX) == 'B', "expected 'B'");
    CHECK("a memory limit that excludes B forces A", choose_design(ops, n, 10000) == 'A', "expected 'A' when B needs %d bytes", 4 * SLOTS);
    CHECK("a limit too small for both returns '?'", choose_design(ops, n, 100) == '?', "expected '?'");
    {
        op_t one[2] = {{OP_INS, 7, 0}, {OP_HAS, 7, 0}};
        CHECK("a tiny workload does not repay B's initialisation", choose_design(one, 2, LONG_MAX) == 'A', "expected 'A'");
        CHECK("no operations: A (tie goes to A)", choose_design(one, 0, LONG_MAX) == 'A', "expected 'A'");
        CHECK("only A fits below B's footprint, with no operations", choose_design(one, 0, 4L * SLOTS - 1) == 'A', "expected 'A'");
    }
    return 0;
}
