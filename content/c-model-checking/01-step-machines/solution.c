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

// Run this: two threads each do "load, add 1, store" on m[0], and we choose the order.
int main(void) {
    static const insn inc[] = { {OP_LOAD,0,0,0,0}, {OP_ADDI,0,1,0,0}, {OP_STORE,0,0,0,0}, {OP_END,0,0,0,0} };
    program p;
    memset(&p, 0, sizeof p);
    p.nthreads = 2; p.code[0] = inc; p.code[1] = inc; p.inv_var = -1; p.fin_lo = -1;

    int order[] = {0, 1, 0, 0, 1, 1};     // thread 1 loads before thread 0 stores
    state s;
    state_init(&p, &s);
    for (int i = 0; i < 6; i++) {
        if (!thread_enabled(&p, &s, order[i])) { printf("step %d: thread %d cannot run\n", i, order[i]); break; }
        step(&p, &s, order[i]);
        printf("after step %d (thread %d): m[0] = %d\n", i, order[i], s.mem[0]);
    }
    return 0;
}
