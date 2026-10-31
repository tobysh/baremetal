#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ---- Simulated cache. Do not edit: it is the instrument, not the thing being fixed. ----
// 64-byte lines, 16 sets, 4 ways, least-recently-used replacement: a 4 KiB cache.
// Every ld()/st() touches the line holding the address; a miss loads the line.
#define LINE 64
#define SETS 16
#define WAYS 4
typedef struct {
    uintptr_t line[SETS][WAYS];
    int valid[SETS][WAYS];
    unsigned long stamp[SETS][WAYS];
    unsigned long clock;
    long accesses, misses;
} cache_t;
static cache_t cache;

__attribute__((unused)) static void cache_reset(void) { memset(&cache, 0, sizeof cache); }

static void cache_touch(const void *p) {
    uintptr_t line = (uintptr_t)p / LINE;
    int s = (int)(line % SETS), victim = 0;
    cache.accesses++;
    cache.clock++;
    for (int w = 0; w < WAYS; w++) {
        if (cache.valid[s][w] && cache.line[s][w] == line) {
            cache.stamp[s][w] = cache.clock;
            return;
        }
    }
    cache.misses++;
    for (int w = 0; w < WAYS; w++) {
        if (!cache.valid[s][w]) { victim = w; break; }
        if (cache.stamp[s][w] < cache.stamp[s][victim]) victim = w;
    }
    cache.valid[s][victim] = 1;
    cache.line[s][victim] = line;
    cache.stamp[s][victim] = cache.clock;
}

__attribute__((unused)) static int ld(const int *p) { cache_touch(p); return *p; }
__attribute__((unused)) static void st(int *p, int v) { cache_touch(p); *p = v; }
// ---- end of cache model ----

#define REGIONS 8

// ---- System allocator wrapper with counters. Do not edit. ----
static long sys_allocs, sys_frees, sys_live;
static void *sys_alloc(size_t n) {
    void *p = aligned_alloc(4096, (n + 4095) / 4096 * 4096);
    if (p) { sys_allocs++; sys_live += (long)n; }
    return p;
}
static void sys_free(void *p, size_t n) { sys_frees++; sys_live -= (long)n; free(p); }
// ---- end of wrapper ----

typedef struct { int id, region, price, qty; int pad[12]; } row_t;   // 64 bytes

typedef struct {
    int n;
    int *price, *qty, *start;     // start has REGIONS + 1 entries
    void *block;
    size_t block_bytes;
} table_t;

static size_t round64(size_t x) { return (x + 63) / 64 * 64; }

// Baseline: reads the rows directly. Do not edit.
long query_aos(const row_t *rows, int n, int region, int min_price) {
    long sum = 0;
    for (int i = 0; i < n; i++) {
        if (ld(&rows[i].region) != region) continue;
        int p = ld(&rows[i].price);
        if (p >= min_price) sum += (long)p * ld(&rows[i].qty);
    }
    return sum;
}

// Build once: one block holding price[], qty[] and start[], sorted by region.
table_t build_table(const row_t *rows, int n) {
    table_t t;
    t.n = n;
    t.block_bytes = 2 * round64(4 * (size_t)n) + round64(4 * (REGIONS + 1));
    t.block = sys_alloc(t.block_bytes);
    t.price = t.block;
    t.qty = (int *)((char *)t.block + round64(4 * (size_t)n));
    t.start = (int *)((char *)t.block + 2 * round64(4 * (size_t)n));
    for (int i = 0; i < n; i++) { t.price[i] = rows[i].price; t.qty[i] = rows[i].qty; }
    for (int r = 0; r <= REGIONS; r++) t.start[r] = n;     // TODO: sort by region and fill start[]
    return t;
}

long query(const table_t *t, int region, int min_price) {
    long sum = 0;
    for (int i = 0; i < t->n; i++) {                       // TODO: only the rows of this region
        int p = ld(&t->price[i]);
        if (p >= min_price) sum += (long)p * ld(&t->qty[i]);
    }
    (void)region;
    return sum;
}

void table_free(table_t *t) {
    sys_free(t->block, t->block_bytes);
    t->block = NULL;
}

static _Alignas(4096) row_t rows[2000];

int main(void) {
    unsigned x = 99;
    for (int i = 0; i < 2000; i++) {
        x = x * 1103515245u + 12345u; rows[i].region = (x >> 8) % REGIONS;
        x = x * 1103515245u + 12345u; rows[i].price = (x >> 8) % 500;
        x = x * 1103515245u + 12345u; rows[i].qty = 1 + (x >> 8) % 9;
        rows[i].id = i;
    }
    table_t t = build_table(rows, 2000);
    cache_reset(); long a = query_aos(rows, 2000, 3, 100); long ma = cache.misses;
    cache_reset(); long b = query(&t, 3, 100);             long mb = cache.misses;
    printf("region 3, price >= 100\n");
    printf("  AoS     sum=%ld misses=%ld\n", a, ma);
    printf("  columns sum=%ld misses=%ld\n", b, mb);
    printf("  system allocations for the table: %ld\n", sys_allocs);
    table_free(&t);
    return 0;
}
