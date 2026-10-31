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
int sa_insert(sorted_set *s, int k) { (void)s; (void)k; return 0; }
int sa_has(sorted_set *s, int k) { (void)s; (void)k; return 0; }
long sa_range(sorted_set *s, int lo, int hi) { (void)s; (void)lo; (void)hi; return 0; }

// ---- Design B: open addressing ----
void hb_init(hash_set *h) { memset(h, 0, sizeof *h); }
int hb_insert(hash_set *h, int k) { (void)h; (void)k; return 0; }
int hb_has(hash_set *h, int k) { (void)h; (void)k; return 0; }
long hb_range(hash_set *h, int lo, int hi) { (void)h; (void)lo; (void)hi; return 0; }

// Write your reasoning here: which workload favours which design, and what the model leaves out.
char choose_design(const op_t *ops, int nops, long mem_limit) {
    (void)ops; (void)nops; (void)mem_limit;
    return 'A';
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
