#include <stdio.h>
#include <string.h>

// ---- Provided model-checking toolkit (do not change this section) ----
#define MAX_THREADS 3
#define NUM_REGS    4
#define MEM_SIZE    8
#define NUM_LOCKS   3
#define MAX_SCHED   64

// One instruction is one atomic step of one thread. r[x] is a register of
// the running thread, m[x] is shared memory.
enum {
    OP_END,     // thread finished; never runnable
    OP_SETI,    // r[a] = b
    OP_MOV,     // r[a] = r[b]
    OP_ADDI,    // r[a] += b
    OP_LOAD,    // r[a] = m[b]
    OP_STORE,   // m[a] = r[b]
    OP_STOREX,  // m[a + r[b]] = r[c]
    OP_SLTI,    // r[a] = (r[b] < c)
    OP_JZ,      // if r[a] == 0 goto b, else next
    OP_JNZ,     // if r[a] != 0 goto b, else next
    OP_JMP,     // goto a
    OP_LOCK,    // lock a: runnable only while the lock is free; owner = thread
    OP_UNLOCK,  // lock a becomes free
    OP_CAS      // if m[a] == r[b] { m[a] = r[c]; r[d] = 1 } else r[d] = 0
};

typedef struct { int op, a, b, c, d; } insn;

typedef struct {
    int nthreads;
    const insn *code[MAX_THREADS];  // each thread's code ends with OP_END
    int init_mem[MEM_SIZE];
    int inv_var, inv_lo, inv_hi;    // after every step inv_lo <= m[inv_var] <= inv_hi; inv_var = -1 disables
    int fin_lo, fin_hi, fin_sum;    // when all threads are done m[fin_lo..fin_hi] sums to fin_sum; fin_lo = -1 disables
} program;

typedef struct {
    int mem[MEM_SIZE];
    int reg[MAX_THREADS][NUM_REGS];
    int pc[MAX_THREADS];
    int lock[NUM_LOCKS];            // 0 = free, otherwise owner thread + 1
} state;

enum { V_OK, V_LOST_UPDATE, V_DEADLOCK, V_ASSERT };

typedef struct {
    int verdict;                    // one of V_*
    int len;                        // number of steps in sched
    int sched[MAX_SCHED];           // thread ids, in order
    int states;                     // distinct states seen (lesson 7)
} result;

static void state_init(const program *p, state *s) {
    memset(s, 0, sizeof *s);
    memcpy(s->mem, p->init_mem, sizeof s->mem);
}

static int all_done(const program *p, const state *s) {
    for (int t = 0; t < p->nthreads; t++)
        if (p->code[t][s->pc[t]].op != OP_END) return 0;
    return 1;
}

static int invariant_ok(const program *p, const state *s) {
    if (p->inv_var < 0) return 1;
    int v = s->mem[p->inv_var];
    return v >= p->inv_lo && v <= p->inv_hi;
}

static int final_ok(const program *p, const state *s) {
    if (p->fin_lo < 0) return 1;
    int sum = 0;
    for (int i = p->fin_lo; i <= p->fin_hi; i++) sum += s->mem[i];
    return sum == p->fin_sum;
}
// ---- End of provided section ----

// Is thread t able to take its next step?
int thread_enabled(const program *p, const state *s, int t) {
    const insn *i = &p->code[t][s->pc[t]];
    if (i->op == OP_END) return 0;
    if (i->op == OP_LOCK) return s->lock[i->a] == 0;
    return 1;
}

// Execute exactly one instruction of thread t (the caller checked thread_enabled).
void step(const program *p, state *s, int t) {
    const insn *i = &p->code[t][s->pc[t]];
    int *r = s->reg[t];
    int next = s->pc[t] + 1;
    switch (i->op) {
    case OP_SETI:   r[i->a] = i->b; break;
    case OP_MOV:    r[i->a] = r[i->b]; break;
    case OP_ADDI:   r[i->a] += i->b; break;
    case OP_LOAD:   r[i->a] = s->mem[i->b]; break;
    case OP_STORE:  s->mem[i->a] = r[i->b]; break;
    case OP_STOREX: s->mem[i->a + r[i->b]] = r[i->c]; break;
    case OP_SLTI:   r[i->a] = r[i->b] < i->c; break;
    case OP_JZ:     if (r[i->a] == 0) next = i->b; break;
    case OP_JNZ:    if (r[i->a] != 0) next = i->b; break;
    case OP_JMP:    next = i->a; break;
    case OP_LOCK:   s->lock[i->a] = t + 1; break;
    case OP_UNLOCK: s->lock[i->a] = 0; break;
    case OP_CAS:
        if (s->mem[i->a] == r[i->b]) { s->mem[i->a] = r[i->c]; r[i->d] = 1; }
        else r[i->d] = 0;
        break;
    default: break;
    }
    s->pc[t] = next;
}

// Run sched from the initial state into *out.
// Returns n if every step was legal, or -(i+1) if step i names a thread that cannot run.
int run_schedule(const program *p, const int *sched, int n, state *out) {
    state_init(p, out);
    for (int i = 0; i < n; i++) {
        int t = sched[i];
        if (t < 0 || t >= p->nthreads || !thread_enabled(p, out, t)) return -(i + 1);
        step(p, out, t);
    }
    return n;
}

static long count_from(const program *p, const state *s) {
    if (all_done(p, s)) return 1;
    long total = 0;
    for (int t = 0; t < p->nthreads; t++) {
        if (!thread_enabled(p, s, t)) continue;
        state next = *s;
        step(p, &next, t);
        total += count_from(p, &next);
    }
    return total;
}

// Number of different schedules that run every thread to completion.
long count_schedules(const program *p) {
    state s;
    state_init(p, &s);
    return count_from(p, &s);
}

static int classify(const program *p, const state *s) {
    if (!invariant_ok(p, s)) return V_ASSERT;
    if (all_done(p, s)) return final_ok(p, s) ? V_OK : V_LOST_UPDATE;
    for (int t = 0; t < p->nthreads; t++)
        if (thread_enabled(p, s, t)) return V_OK;
    return V_DEADLOCK;
}

#define MAX_STATES 40000
#define HASH_SIZE  131072

typedef struct { state s; int parent, tid, depth; } node;
static node nodes[MAX_STATES];
static int table[HASH_SIZE];            // node index + 1, 0 = empty

static unsigned hash_state(const state *s) {
    const unsigned char *b = (const unsigned char *)s;
    unsigned h = 2166136261u;
    for (size_t i = 0; i < sizeof *s; i++) { h ^= b[i]; h *= 16777619u; }
    return h;
}

// Index of s in the table, or -1 after inserting it as node `count`.
static int lookup_or_insert(const state *s, int count) {
    unsigned i = hash_state(s) & (HASH_SIZE - 1);
    while (table[i]) {
        if (memcmp(&nodes[table[i] - 1].s, s, sizeof *s) == 0) return table[i] - 1;
        i = (i + 1) & (HASH_SIZE - 1);
    }
    table[i] = count + 1;
    return -1;
}

// Breadth-first search over distinct states. Nodes leave the queue in
// (length, lexicographic) order, so the first failing node is the minimal counterexample.
void model_check(const program *p, result *r) {
    memset(r, 0, sizeof *r);
    memset(table, 0, sizeof table);
    int count = 1;
    state_init(p, &nodes[0].s);
    nodes[0].parent = -1; nodes[0].tid = -1; nodes[0].depth = 0;
    lookup_or_insert(&nodes[0].s, 0);
    for (int head = 0; head < count; head++) {
        int v = classify(p, &nodes[head].s);
        if (v != V_OK) {
            r->verdict = v;
            r->len = nodes[head].depth;
            int k = head;
            for (int i = r->len - 1; i >= 0; i--) { r->sched[i] = nodes[k].tid; k = nodes[k].parent; }
            break;
        }
        if (nodes[head].depth >= MAX_SCHED) continue;
        for (int t = 0; t < p->nthreads; t++) {
            if (!thread_enabled(p, &nodes[head].s, t) || count >= MAX_STATES) continue;
            state next = nodes[head].s;
            step(p, &next, t);
            if (lookup_or_insert(&next, count) >= 0) continue;
            nodes[count].s = next;
            nodes[count].parent = head; nodes[count].tid = t; nodes[count].depth = nodes[head].depth + 1;
            count++;
        }
    }
    r->states = count;
}

// Memory layout of the push-only ring: m[0] = tail index, m[1..3] = slots.
// Every thread pushes the value 1. Success means all n values landed in different slots,
// so the slots sum to n. A lost push leaves a smaller sum.
static void push_base(program *p, int n) {
    memset(p, 0, sizeof *p);
    p->nthreads = n;
    p->inv_var = -1;
    p->fin_lo = 1; p->fin_hi = 3; p->fin_sum = n;
}

// Lock-based: the lock covers read of tail, slot write and tail update.
static const insn LOCKED_PUSH[] = {
    {OP_LOCK, 0, 0, 0, 0},
    {OP_LOAD, 0, 0, 0, 0},          // r0 = tail
    {OP_SETI, 1, 1, 0, 0},          // r1 = 1
    {OP_STOREX, 1, 0, 1, 0},        // m[1 + r0] = r1
    {OP_ADDI, 0, 1, 0, 0},
    {OP_STORE, 0, 0, 0, 0},         // tail = r0 + 1
    {OP_UNLOCK, 0, 0, 0, 0},
    {OP_END, 0, 0, 0, 0},
};

// Flawed lock-free: read tail, write the slot, then store tail + 1 with a plain store.
static const insn FLAWED_PUSH[] = {
    {OP_LOAD, 0, 0, 0, 0},
    {OP_SETI, 1, 1, 0, 0},
    {OP_STOREX, 1, 0, 1, 0},
    {OP_ADDI, 0, 1, 0, 0},
    {OP_STORE, 0, 0, 0, 0},
    {OP_END, 0, 0, 0, 0},
};

// Fixed lock-free: claim the slot index with CAS on tail, retry if someone else won, then write.
static const insn CAS_PUSH[] = {
    {OP_LOAD, 0, 0, 0, 0},          // r0 = tail
    {OP_MOV, 2, 0, 0, 0},
    {OP_ADDI, 2, 1, 0, 0},          // r2 = r0 + 1
    {OP_CAS, 0, 0, 2, 3},           // r3 = (tail == r0) and tail = r2
    {OP_JZ, 3, 0, 0, 0},            // lost the race: start over
    {OP_SETI, 1, 1, 0, 0},
    {OP_STOREX, 1, 0, 1, 0},        // the slot r0 is ours alone
    {OP_END, 0, 0, 0, 0},
};

void build_locked_push(program *p, int n) {
    push_base(p, n);
    for (int t = 0; t < n; t++) p->code[t] = LOCKED_PUSH;
}
void build_flawed_push(program *p, int n) {
    push_base(p, n);
    for (int t = 0; t < n; t++) p->code[t] = FLAWED_PUSH;
}
void build_cas_push(program *p, int n) {
    push_base(p, n);
    for (int t = 0; t < n; t++) p->code[t] = CAS_PUSH;
}

static const char *verdict_name(int v) {
    static const char *names[] = { "OK", "LOST_UPDATE", "DEADLOCK", "ASSERT" };
    return names[v];
}

// Run this: check every version with 2 pushing threads.
int main(void) {
    program p;
    result r;
    for (int which = 0; which < 3; which++) {
        static const char *name[] = { "locked", "flawed lock-free", "CAS lock-free" };
        if (which == 0) build_locked_push(&p, 2);
        else if (which == 1) build_flawed_push(&p, 2);
        else build_cas_push(&p, 2);
        model_check(&p, &r);
        printf("%-17s %-12s states=%d", name[which], verdict_name(r.verdict), r.states);
        if (r.verdict != V_OK) {
            printf("  schedule:");
            for (int i = 0; i < r.len; i++) printf(" %d", r.sched[i]);
        }
        printf("\n");
    }
    return 0;
}
