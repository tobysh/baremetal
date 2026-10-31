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

#define NP 128
typedef struct { int id, x, y, z, vx, vy, vz, mass; int tag[8]; } particle;   // 64 bytes

static _Alignas(4096) particle ps[NP];
static _Alignas(4096) int sx[NP];     // SoA copy of ps[i].x
static _Alignas(4096) int svx[NP];    // SoA copy of ps[i].vx

// Array-of-structs step: every particle's line is loaded.
void advance_x_aos(void) {
    for (int i = 0; i < NP; i++) st(&ps[i].x, ld(&ps[i].x) + ld(&ps[i].vx));
}

// Setup, not modelled: copy the hot fields into their own arrays.
void soa_load(void) {
    for (int i = 0; i < NP; i++) { sx[i] = ps[i].x; svx[i] = ps[i].vx; }
}

// Struct-of-arrays step. Same result as advance_x_aos, on sx and svx.
void advance_x_soa(void) {
    for (int i = 0; i < NP; i++) st(&ps[i].x, ld(&ps[i].x) + ld(&ps[i].vx));
}

int main(void) {
    for (int i = 0; i < NP; i++) { ps[i].id = i; ps[i].x = i; ps[i].vx = 2; }
    cache_reset(); advance_x_aos();
    printf("AoS step: accesses=%ld misses=%ld\n", cache.accesses, cache.misses);
    soa_load();
    cache_reset(); advance_x_soa();
    printf("SoA step: accesses=%ld misses=%ld\n", cache.accesses, cache.misses);
    printf("sx[5] = %d (AoS ps[5].x = %d)\n", sx[5], ps[5].x);
    return 0;
}
