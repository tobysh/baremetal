// Checks that the cache model was not edited, using a hand-computed sequence.
static int cache_model_ok(void) {
    static _Alignas(4096) int a[4096];
    cache_reset();
    for (int i = 0; i < 16; i++) cache_touch(&a[i]);          // one line: 1 miss, 15 hits
    if (cache.misses != 1 || cache.accesses != 16) return 0;
    for (int k = 0; k < 5; k++) cache_touch(&a[k * 256]);      // 5 lines of one set (1 KiB apart); 4 ways
    if (cache.misses != 5) return 0;                           // a[0] hit, 4 new misses
    cache_touch(&a[0]);                                        // a[0] was least recently used and is gone
    if (cache.misses != 6) return 0;
    cache_touch(&a[256 * 4]);                                  // still resident
    return cache.misses == 6;
}

int main(void) {
    CHECK("the cache model has not been edited", cache_model_ok(), "restore the cache block");

    static int x0[NP], v0[NP];
    int bad_val = 0, bad_v = 0, bad_ps = 0;
    long worst_miss = 0, bad_acc = 0;
    for (unsigned seed = 1; seed <= 5; seed++) {
        unsigned r = seed * 40503u;
        memset(ps, 0, sizeof ps);
        for (int i = 0; i < NP; i++) {
            r = r * 1103515245u + 12345u; x0[i] = (int)((r >> 8) % 20001) - 10000;
            r = r * 1103515245u + 12345u; v0[i] = (int)((r >> 8) % 201) - 100;
            ps[i].id = i; ps[i].x = x0[i]; ps[i].vx = v0[i]; ps[i].y = 77 + i;
        }
        soa_load();
        int steps = (int)seed;
        for (int s = 0; s < steps; s++) {
            cache_reset();
            advance_x_soa();
            if (cache.misses > worst_miss) worst_miss = cache.misses;
            if (cache.accesses != 3 * NP) bad_acc = cache.accesses;
        }
        for (int i = 0; i < NP; i++) {
            if (sx[i] != x0[i] + steps * v0[i]) bad_val++;
            if (svx[i] != v0[i]) bad_v++;
            if (ps[i].x != x0[i] || ps[i].y != 77 + i) bad_ps++;
        }
    }
    CHECK("positions advance by the velocity once per step", bad_val == 0, "%d positions differ from x + steps * vx", bad_val);
    CHECK("velocities are left alone", bad_v == 0, "%d velocities changed", bad_v);
    CHECK("the SoA step does not touch the particle array", bad_ps == 0, "ps was modified or read-modified %d times; use sx and svx", bad_ps);
    CHECK("three accesses per particle", bad_acc == 0, "a step made %ld accesses, expected %d", bad_acc, 3 * NP);
    CHECK("at most 16 misses per step", worst_miss <= 16, "%ld misses; the arrays are 8 lines each, so 16 is the minimum", worst_miss);

    // The AoS version is the baseline the lesson quotes; it must still cost what the lesson says.
    for (int i = 0; i < NP; i++) { ps[i].x = i; ps[i].vx = 1; }
    cache_reset(); advance_x_aos();
    CHECK("the AoS baseline is unchanged (128 misses)", cache.misses == 128 && ps[9].x == 10, "AoS made %ld misses", cache.misses);
    return 0;
}
