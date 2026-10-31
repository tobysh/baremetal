#include <stdio.h>
#include <string.h>
#include <limits.h>

#define MAXN 2048
#define SLOTS 8192

// Cost instrument. You must maintain it honestly in your code.
typedef struct { long probes, moves, init; } cost_t;

typedef struct { int key[MAXN]; int n; cost_t c; } sorted_set;    // Design A
typedef struct { int slot[SLOTS]; int n; cost_t c; } hash_set;    // Design B, 0 = empty

static unsigned hash_key(int k) { return ((unsigned)k * 2654435761u) >> 19; }   // 0..8191

enum { OP_INS, OP_HAS, OP_RANGE };
typedef struct { int kind, a, b; } op_t;     // a = key (or lo), b = hi for ranges

// ---- Design A: sorted array ----
void sa_init(sorted_set *s) { memset(s, 0, sizeof *s); }
// First index whose key is >= k, one probe per step.
static int sa_lower(sorted_set *s, int k) {
    int lo = 0, hi = s->n;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        s->c.probes++;
        if (s->key[mid] < k) lo = mid + 1; else hi = mid;
    }
    return lo;
}
int sa_insert(sorted_set *s, int k) {
    int i = sa_lower(s, k);
    if (i < s->n && s->key[i] == k) return 0;
    for (int j = s->n; j > i; j--) { s->key[j] = s->key[j - 1]; s->c.moves++; }
    s->key[i] = k;
    s->n++;
    return 1;
}
int sa_has(sorted_set *s, int k) {
    int i = sa_lower(s, k);
    return i < s->n && s->key[i] == k;
}
long sa_range(sorted_set *s, int lo, int hi) {
    if (lo > hi) return 0;
    return sa_lower(s, hi + 1) - sa_lower(s, lo);     // keys are ints; hi + 1 is the first value past the range
}

// ---- Design B: open addressing ----
void hb_init(hash_set *h) { memset(h, 0, sizeof *h); h->c.init = SLOTS; }
int hb_insert(hash_set *h, int k) {
    unsigned i = hash_key(k);
    for (;;) {
        h->c.probes++;
        if (h->slot[i] == 0) { h->slot[i] = k; h->n++; return 1; }
        if (h->slot[i] == k) return 0;
        i = (i + 1) % SLOTS;
    }
}
int hb_has(hash_set *h, int k) {
    unsigned i = hash_key(k);
    for (;;) {
        h->c.probes++;
        if (h->slot[i] == 0) return 0;
        if (h->slot[i] == k) return 1;
        i = (i + 1) % SLOTS;
    }
}
long hb_range(hash_set *h, int lo, int hi) {
    long cnt = 0;
    for (int i = 0; i < SLOTS; i++) {
        h->c.probes++;
        if (h->slot[i] != 0 && h->slot[i] >= lo && h->slot[i] <= hi) cnt++;
    }
    return cnt;
}

// Write your reasoning here: which workload favours which design, and what the model leaves out.
// B wins on inserts and lookups once there is enough work to repay its 8192-slot init;
// A wins on range queries, on tiny workloads and when memory is tight. Neither model
// counts cache effects, so near-ties should be re-checked on the target machine.
char choose_design(const op_t *ops, int nops, long mem_limit) {
    static sorted_set a;
    static hash_set b;
    sa_init(&a); hb_init(&b);
    for (int i = 0; i < nops; i++) {
        if (ops[i].kind == OP_INS) { sa_insert(&a, ops[i].a); hb_insert(&b, ops[i].a); }
        else if (ops[i].kind == OP_HAS) { sa_has(&a, ops[i].a); hb_has(&b, ops[i].a); }
        else { sa_range(&a, ops[i].a, ops[i].b); hb_range(&b, ops[i].a, ops[i].b); }
    }
    long ca = a.c.probes + a.c.moves + a.c.init, cb = b.c.probes + b.c.moves + b.c.init;
    int okA = 4L * a.n <= mem_limit, okB = 4L * SLOTS <= mem_limit;
    if (!okA && !okB) return '?';
    if (!okB) return 'A';
    if (!okA) return 'B';
    return ca <= cb ? 'A' : 'B';
}

static sorted_set sa;
static hash_set hb;

int main(void) {
    sa_init(&sa); hb_init(&hb);
    int keys[] = {50, 20, 80, 20, 65, 5};
    for (int i = 0; i < 6; i++) {
        int x = sa_insert(&sa, keys[i]), y = hb_insert(&hb, keys[i]);
        printf("insert %2d: A=%d B=%d\n", keys[i], x, y);
    }
    printf("range 20..65: A=%ld B=%ld (expected 3)\n", sa_range(&sa, 20, 65), hb_range(&hb, 20, 65));
    printf("cost A: probes=%ld moves=%ld init=%ld\n", sa.c.probes, sa.c.moves, sa.c.init);
    printf("cost B: probes=%ld moves=%ld init=%ld\n", hb.c.probes, hb.c.moves, hb.c.init);
    return 0;
}
