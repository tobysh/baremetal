#include <stdio.h>
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

#define N 64
static _Alignas(4096) int grid[N][N];

// out[c] = sum of column c. out[] is a few accumulators; we treat it as registers.
void col_sums(long out[N]) {
    for (int c = 0; c < N; c++) out[c] = 0;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++) out[c] += ld(&grid[r][c]);
}

// Fills the grid, runs col_sums on a cold cache and prints the model's counters.
int main(void) {
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++) grid[r][c] = r * N + c;
    long out[N];
    cache_reset();
    col_sums(out);
    printf("column 0 sum  = %ld\n", out[0]);
    printf("accesses      = %ld\n", cache.accesses);
    printf("misses        = %ld\n", cache.misses);
    return 0;
}
