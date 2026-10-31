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

static _Alignas(4096) int data[1024];

// Reads data[0], data[stride], ... below n on a cold cache and returns the misses.
long scan(long n, long stride) {
    cache_reset();
    long sink = 0;
    for (long i = 0; i < n; i += stride) sink += ld(&data[i]);
    (void)sink;
    return cache.misses;
}

// Your predictions. Arithmetic only.
long predict_misses(long n, long stride) {
    if (n <= 0) return 0;
    long reads = (n + stride - 1) / stride;
    if (stride * 4 >= LINE) return reads;          // every read is on its own line
    long last = (reads - 1) * stride;              // no line is skipped below one line per step
    return last * 4 / LINE + 1;
}

int sets_touched(long stride_lines) {
    long a = SETS, b = stride_lines % SETS;
    while (b) { long t = a % b; a = b; b = t; }
    return (int)(SETS / a);
}

static long gcd_l(long a, long b) { return b ? gcd_l(b, a % b) : a; }

int main(void) {
    printf("n=256   stride  measured  predicted\n");
    long strides[] = {1, 2, 4, 8, 16, 32};
    for (int i = 0; i < 6; i++)
        printf("        %6ld  %8ld  %9ld\n", strides[i], scan(256, strides[i]), predict_misses(256, strides[i]));
    printf("stride in lines -> sets touched\n");
    for (long s = 1; s <= 4; s++) printf("  %ld -> %d (gcd rule: %ld)\n", s, sets_touched(s), SETS / gcd_l(SETS, s));
    return 0;
}
