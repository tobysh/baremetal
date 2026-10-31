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
static int same_as_ref(const program *p, const int *sched, int n) {
    state a; rstate b;
    int ra = run_schedule(p, sched, n, &a);
    int rb = r_replay(p, sched, n, &b) < 0 ? -1 : n;
    if (rb < 0) return ra < 0;
    return ra == n && memcmp(&a, &b, sizeof a) == 0;
}

int main(void) {
    program p = mk(2, INC_RACY, INC_RACY, 0);
    state s;
    int n;

    int serial[] = {0,0,0,1,1,1};
    n = run_schedule(&p, serial, 6, &s);
    CHECK("a legal schedule returns its length", n == 6, "returned %d, expected 6", n);
    CHECK("serial increments reach 2", s.mem[0] == 2, "mem[0] is %d, expected 2", s.mem[0]);

    int race[] = {0,1,0,0,1,1};
    n = run_schedule(&p, race, 6, &s);
    CHECK("an interleaved schedule loses an update", n == 6 && s.mem[0] == 1, "returned %d, mem[0] = %d, expected 6 and 1", n, s.mem[0]);

    n = run_schedule(&p, race, 0, &s);
    CHECK("an empty schedule leaves the initial state", n == 0 && s.mem[0] == 0 && s.pc[0] == 0, "returned %d, mem[0]=%d", n, s.mem[0]);

    int prefix[] = {1,1};
    n = run_schedule(&p, prefix, 2, &s);
    CHECK("a prefix is allowed and stops where it ends", n == 2 && s.pc[1] == 2 && s.pc[0] == 0, "returned %d, pc = %d/%d", n, s.pc[0], s.pc[1]);

    int tooLong[] = {0,0,0,0};
    n = run_schedule(&p, tooLong, 4, &s);
    CHECK("stepping a finished thread is reported at its position", n == -4, "returned %d, expected -4", n);

    int bad[] = {0,2};
    n = run_schedule(&p, bad, 2, &s);
    CHECK("an unknown thread id is rejected", n == -2, "returned %d, expected -2", n);

    program d = mk(2, DL_AB, DL_BA, 0);
    int blocked[] = {0,1,0};
    n = run_schedule(&d, blocked, 3, &s);
    CHECK("a blocked LOCK is not a legal step", n == -3, "returned %d, expected -3", n);

    program L = mk(2, INC_LOCKED, INC_LOCKED, 0);
    int lk[] = {0,1};
    n = run_schedule(&L, lk, 2, &s);
    CHECK("the second thread cannot enter the critical section", n == -2, "returned %d, expected -2", n);

    // random schedules agree with the reference, including illegal ones
    int ok = 1;
    unsigned seed = 99;
    program set[3] = { mk(2, WITHDRAW, WITHDRAW, 0), mk(3, INC_LOCKED, INC_RACY, INC_LOCKED), mk(2, DL_AB, DL_BA, 0) };
    set[0].init_mem[0] = 100;
    for (int k = 0; k < 300 && ok; k++) {
        int sc[12];
        seed = seed * 1103515245u + 12345u;
        int pi = (seed >> 16) % 3;
        for (int i = 0; i < 12; i++) { seed = seed * 1103515245u + 12345u; sc[i] = (seed >> 16) % set[pi].nthreads; }
        ok = same_as_ref(&set[pi], sc, 12);
    }
    CHECK("random schedules agree with the reference", ok, "run_schedule disagreed with the reference on a random schedule");

    p = mk(2, INC_RACY, INC_RACY, 0);
    CHECK("two 3-step threads: 20 schedules", count_schedules(&p) == 20, "got %ld, expected C(6,3) = 20", count_schedules(&p));
    p = mk(3, INC_RACY, INC_RACY, INC_RACY);
    CHECK("three 3-step threads: 1680 schedules", count_schedules(&p) == 1680, "got %ld, expected 9!/(3!3!3!) = 1680", count_schedules(&p));
    p = mk(2, INC_RACY, INC2_RACY, 0);
    CHECK("threads of 3 and 6 steps: 84 schedules", count_schedules(&p) == 84, "got %ld, expected C(9,3) = 84", count_schedules(&p));
    p = mk(1, INC2_RACY, 0, 0);
    CHECK("one thread has exactly one schedule", count_schedules(&p) == 1, "got %ld", count_schedules(&p));
    p = mk(2, INC_LOCKED, INC_LOCKED, 0);
    rstate r0; r_init(&p, &r0);
    CHECK("locks remove schedules", count_schedules(&p) == r_count(&p, &r0), "got %ld, expected %ld", count_schedules(&p), r_count(&p, &r0));
    p = mk(3, INC2_LOCKED, INC_LOCKED, INC_RACY);
    r_init(&p, &r0);
    CHECK("a mix of locked and unlocked threads", count_schedules(&p) == r_count(&p, &r0), "got %ld, expected %ld", count_schedules(&p), r_count(&p, &r0));
    p = mk(2, DL_AB, DL_BA, 0);
    r_init(&p, &r0);
    CHECK("dead ends are not complete schedules", count_schedules(&p) == r_count(&p, &r0), "got %ld, expected %ld", count_schedules(&p), r_count(&p, &r0));
    return 0;
}
