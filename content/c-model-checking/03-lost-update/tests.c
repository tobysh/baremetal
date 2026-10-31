// ===== Independent reference (the learner's step/model_check are never used here) =====
typedef struct { int mem[MEM_SIZE]; int reg[MAX_THREADS][NUM_REGS]; int pc[MAX_THREADS]; int lock[NUM_LOCKS]; } rstate;

static void r_init(const program *p, rstate *s) {
    memset(s, 0, sizeof *s);
    for (int i = 0; i < MEM_SIZE; i++) s->mem[i] = p->init_mem[i];
}
static int r_op(const program *p, const rstate *s, int t) { return p->code[t][s->pc[t]].op; }
static int r_enabled(const program *p, const rstate *s, int t) {
    int op = r_op(p, s, t);
    if (op == OP_END) return 0;
    if (op == OP_LOCK && s->lock[p->code[t][s->pc[t]].a] != 0) return 0;
    return 1;
}
static void r_step(const program *p, rstate *s, int t) {
    insn in = p->code[t][s->pc[t]];
    int *R = s->reg[t];
    int pc = s->pc[t] + 1;
    if (in.op == OP_SETI) R[in.a] = in.b;
    else if (in.op == OP_MOV) R[in.a] = R[in.b];
    else if (in.op == OP_ADDI) R[in.a] = R[in.a] + in.b;
    else if (in.op == OP_LOAD) R[in.a] = s->mem[in.b];
    else if (in.op == OP_STORE) s->mem[in.a] = R[in.b];
    else if (in.op == OP_STOREX) s->mem[in.a + R[in.b]] = R[in.c];
    else if (in.op == OP_SLTI) R[in.a] = (R[in.b] < in.c) ? 1 : 0;
    else if (in.op == OP_JZ) { if (!R[in.a]) pc = in.b; }
    else if (in.op == OP_JNZ) { if (R[in.a]) pc = in.b; }
    else if (in.op == OP_JMP) pc = in.a;
    else if (in.op == OP_LOCK) s->lock[in.a] = t + 1;
    else if (in.op == OP_UNLOCK) s->lock[in.a] = 0;
    else if (in.op == OP_CAS) {
        int ok = (s->mem[in.a] == R[in.b]);
        if (ok) s->mem[in.a] = R[in.c];
        R[in.d] = ok;
    }
    s->pc[t] = pc;
}
static int r_done(const program *p, const rstate *s) {
    int n = 0;
    for (int t = 0; t < p->nthreads; t++) n += (r_op(p, s, t) == OP_END);
    return n == p->nthreads;
}
static int r_any_enabled(const program *p, const rstate *s) {
    for (int t = 0; t < p->nthreads; t++) if (r_enabled(p, s, t)) return 1;
    return 0;
}
static int r_class(const program *p, const rstate *s) {
    if (p->inv_var >= 0 && (s->mem[p->inv_var] < p->inv_lo || s->mem[p->inv_var] > p->inv_hi)) return V_ASSERT;
    if (r_done(p, s)) {
        if (p->fin_lo < 0) return V_OK;
        int sum = 0;
        for (int i = p->fin_lo; i <= p->fin_hi; i++) sum += s->mem[i];
        return sum == p->fin_sum ? V_OK : V_LOST_UPDATE;
    }
    return r_any_enabled(p, s) ? V_OK : V_DEADLOCK;
}

// Replays sched; returns the class of the final state, or -1 if some step is illegal.
static int r_replay(const program *p, const int *sched, int n, rstate *out) {
    r_init(p, out);
    for (int i = 0; i < n; i++) {
        if (sched[i] < 0 || sched[i] >= p->nthreads || !r_enabled(p, out, sched[i])) return -1;
        r_step(p, out, sched[i]);
    }
    return r_class(p, out);
}

// Exhaustive enumeration (acyclic programs only): shortest failing schedule, lexicographically first.
typedef struct { int best_len, verdict; int best[MAX_SCHED]; int cur[MAX_SCHED]; } rbest;
static void r_enum(const program *p, const rstate *s, int depth, rbest *b) {
    if (depth >= b->best_len || depth >= MAX_SCHED) return;
    int v = r_class(p, s);
    if (v != V_OK) {
        b->best_len = depth; b->verdict = v;
        memcpy(b->best, b->cur, sizeof b->cur);
        return;
    }
    for (int t = 0; t < p->nthreads; t++) {
        if (!r_enabled(p, s, t)) continue;
        rstate n = *s;
        r_step(p, &n, t);
        b->cur[depth] = t;
        r_enum(p, &n, depth + 1, b);
    }
}
static void r_minimal(const program *p, rbest *b) {
    b->best_len = MAX_SCHED + 1; b->verdict = V_OK;
    memset(b->best, 0, sizeof b->best); memset(b->cur, 0, sizeof b->cur);
    rstate s; r_init(p, &s);
    r_enum(p, &s, 0, b);
    if (b->verdict == V_OK) b->best_len = 0;
}

// Reachable-state exploration with a different structure: visited list plus work stack.
#define R_CAP 30000
static rstate r_seen[R_CAP];
static int r_nseen;
static int r_stack[R_CAP];
static int r_find(const rstate *s) {
    for (int i = 0; i < r_nseen; i++) if (memcmp(&r_seen[i], s, sizeof *s) == 0) return i;
    return -1;
}
static int r_reach(const program *p) {
    r_nseen = 0;
    int sp = 0;
    r_init(p, &r_seen[0]);
    r_nseen = 1; r_stack[sp++] = 0;
    while (sp > 0) {
        rstate cur = r_seen[r_stack[--sp]];
        for (int t = 0; t < p->nthreads; t++) {
            if (!r_enabled(p, &cur, t)) continue;
            rstate n = cur;
            r_step(p, &n, t);
            if (r_find(&n) < 0 && r_nseen < R_CAP) { r_seen[r_nseen] = n; r_stack[sp++] = r_nseen; r_nseen++; }
        }
    }
    return r_nseen;
}
// Does any reachable state fail? (for cyclic programs; verdict only)
static int r_any_failure(const program *p) {
    r_reach(p);
    for (int i = 0; i < r_nseen; i++) if (r_class(p, &r_seen[i]) != V_OK) return r_class(p, &r_seen[i]);
    return V_OK;
}
static long r_count(const program *p, const rstate *s) {
    if (r_done(p, s)) return 1;
    long n = 0;
    for (int t = 0; t < p->nthreads; t++) {
        if (!r_enabled(p, s, t)) continue;
        rstate x = *s; r_step(p, &x, t); n += r_count(p, &x);
    }
    return n;
}

// ===== Test programs =====
#define E {OP_END,0,0,0,0}
static const insn INC_RACY[]   = { {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0}, E };
static const insn INC_LOCKED[] = { {OP_LOCK,0,0,0,0}, {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn INC2_RACY[]  = { {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0},
                                   {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0}, E };
static const insn INC2_LOCKED[]= { {OP_LOCK,0,0,0,0}, {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0},
                                   {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn DL_AB[] = { {OP_LOCK,0,0,0,0}, {OP_LOCK,1,0,0,0}, {OP_UNLOCK,1,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn DL_BA[] = { {OP_LOCK,1,0,0,0}, {OP_LOCK,0,0,0,0}, {OP_UNLOCK,0,0,0,0}, {OP_UNLOCK,1,0,0,0}, E };
static const insn RING_01[] = { {OP_LOCK,0,0,0,0}, {OP_LOCK,1,0,0,0}, {OP_UNLOCK,1,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn RING_12[] = { {OP_LOCK,1,0,0,0}, {OP_LOCK,2,0,0,0}, {OP_UNLOCK,2,0,0,0}, {OP_UNLOCK,1,0,0,0}, E };
static const insn RING_20[] = { {OP_LOCK,2,0,0,0}, {OP_LOCK,0,0,0,0}, {OP_UNLOCK,0,0,0,0}, {OP_UNLOCK,2,0,0,0}, E };
static const insn RING_02[] = { {OP_LOCK,0,0,0,0}, {OP_LOCK,2,0,0,0}, {OP_UNLOCK,2,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn LEAK_A[] = { {OP_LOCK,0,0,0,0}, E };
static const insn LEAK_B[] = { {OP_LOCK,0,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn WITHDRAW[] = { {OP_LOAD,0,0,0,0}, {OP_SLTI,1,0,60,0}, {OP_JNZ,1,6,0,0}, {OP_LOAD,0,0,0,0},
                                 {OP_ADDI,0,-60,0,0}, {OP_STORE,0,0,0,0}, E };
static const insn WITHDRAW_L[] = { {OP_LOCK,0,0,0,0}, {OP_LOAD,0,0,0,0}, {OP_SLTI,1,0,60,0}, {OP_JNZ,1,7,0,0},
                                   {OP_LOAD,0,0,0,0}, {OP_ADDI,0,-60,0,0}, {OP_STORE,0,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn TRANSIENT[] = { {OP_LOAD,0,0,0,0}, {OP_ADDI,0,-150,0,0}, {OP_STORE,0,0,0,0},
                                  {OP_ADDI,0,150,0,0}, {OP_STORE,0,0,0,0}, E };
static const insn MIX_A[] = { {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0},
                              {OP_LOCK,0,0,0,0}, {OP_LOCK,1,0,0,0}, {OP_UNLOCK,1,0,0,0}, {OP_UNLOCK,0,0,0,0}, E };
static const insn MIX_B[] = { {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0},
                              {OP_LOCK,1,0,0,0}, {OP_LOCK,0,0,0,0}, {OP_UNLOCK,0,0,0,0}, {OP_UNLOCK,1,0,0,0}, E };
static const insn CAS_INC[] = { {OP_LOAD,0,0,0,0}, {OP_MOV,1,0,0,0}, {OP_ADDI,1,1,0,0}, {OP_CAS,0,0,1,2},
                                {OP_JZ,2,0,0,0}, E };
static const insn CAS_INC2[] = { {OP_LOAD,0,0,0,0}, {OP_MOV,1,0,0,0}, {OP_ADDI,1,1,0,0}, {OP_CAS,0,0,1,2}, {OP_JZ,2,0,0,0},
                                 {OP_LOAD,0,0,0,0}, {OP_MOV,1,0,0,0}, {OP_ADDI,1,1,0,0}, {OP_CAS,0,0,1,2}, {OP_JZ,2,5,0,0}, E };
#undef E

static program mk(int n, const insn *a, const insn *b, const insn *c) {
    program p;
    memset(&p, 0, sizeof p);
    p.nthreads = n;
    p.code[0] = a; p.code[1] = b; p.code[2] = c;
    p.inv_var = -1;
    p.fin_lo = -1;
    return p;
}
static program mk_counter(int n, const insn *code) {       // n copies of code; mem[0] must end as n * per
    program p = mk(n, code, code, code);
    return p;
}
static void expect_sum(program *p, int lo, int hi, int sum) { p->fin_lo = lo; p->fin_hi = hi; p->fin_sum = sum; }
static void expect_range(program *p, int var, int lo, int hi) { p->inv_var = var; p->inv_lo = lo; p->inv_hi = hi; }

static int sched_str(char *buf, int cap, const int *s, int n) {
    int w = 0;
    buf[0] = 0;
    for (int i = 0; i < n && w + 4 < cap; i++) w += snprintf(buf + w, cap - w, "%d", s[i]);
    return w;
}
// A counterexample is accepted only if the reference replays it to a failing state of the claimed kind.
static int valid(const program *p, const result *r, int kind, char *why, int cap) {
    if (r->verdict != kind) { snprintf(why, cap, "verdict %d, expected %d", r->verdict, kind); return 0; }
    if (r->len < 0 || r->len > MAX_SCHED) { snprintf(why, cap, "len %d out of range", r->len); return 0; }
    rstate s;
    int c = r_replay(p, r->sched, r->len, &s);
    if (c < 0) { char b[200]; sched_str(b, sizeof b, r->sched, r->len); snprintf(why, cap, "schedule %s is not legal", b); return 0; }
    if (c != kind) { char b[200]; sched_str(b, sizeof b, r->sched, r->len); snprintf(why, cap, "schedule %s does not end in a failing state of that kind", b); return 0; }
    return 1;
}
static int clean(const result *r) { return r->verdict == V_OK; }

int main(void) {
    char why[260] = "";
    result r;
    program p;

    p = mk_counter(2, INC_RACY); expect_sum(&p, 0, 0, 2);
    model_check(&p, &r);
    CHECK("two racy increments lose an update", valid(&p, &r, V_LOST_UPDATE, why, sizeof why), "%s", why);

    p = mk_counter(3, INC_RACY); expect_sum(&p, 0, 0, 3);
    model_check(&p, &r);
    CHECK("three racy increments lose an update", valid(&p, &r, V_LOST_UPDATE, why, sizeof why), "%s", why);

    p = mk(2, INC2_RACY, INC2_RACY, 0); expect_sum(&p, 0, 0, 4);
    model_check(&p, &r);
    CHECK("two increments per thread", valid(&p, &r, V_LOST_UPDATE, why, sizeof why), "%s", why);

    p = mk_counter(2, INC_LOCKED); expect_sum(&p, 0, 0, 2);
    model_check(&p, &r);
    CHECK("the locked counter has no failing schedule", clean(&r), "verdict %d, expected V_OK", r.verdict);

    p = mk_counter(3, INC_LOCKED); expect_sum(&p, 0, 0, 3);
    model_check(&p, &r);
    CHECK("three locked threads are fine", clean(&r), "verdict %d, expected V_OK", r.verdict);

    p = mk(2, INC2_LOCKED, INC_LOCKED, 0); expect_sum(&p, 0, 0, 3);
    model_check(&p, &r);
    CHECK("locked threads of different length are fine", clean(&r), "verdict %d, expected V_OK", r.verdict);

    p = mk(1, INC2_RACY, 0, 0); expect_sum(&p, 0, 0, 2);
    model_check(&p, &r);
    CHECK("one thread cannot race with itself", clean(&r), "verdict %d, expected V_OK", r.verdict);

    p = mk_counter(2, INC_RACY);
    model_check(&p, &r);
    CHECK("with no final-state expectation nothing is checked", clean(&r), "verdict %d, expected V_OK", r.verdict);

    p = mk(2, INC_RACY, INC_LOCKED, 0); expect_sum(&p, 0, 0, 2);
    model_check(&p, &r);
    CHECK("a lock held by only one side does not help", valid(&p, &r, V_LOST_UPDATE, why, sizeof why), "%s", why);

    p = mk_counter(2, INC_LOCKED); expect_sum(&p, 0, 0, 5);
    model_check(&p, &r);
    CHECK("a wrong expectation fails even without a race", valid(&p, &r, V_LOST_UPDATE, why, sizeof why), "%s", why);
    return 0;
}
