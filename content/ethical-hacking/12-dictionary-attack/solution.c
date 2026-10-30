#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <math.h>

// Provided toy hash. NOT secure; for counting work only.
uint32_t toy_hash(const char *salt, const char *pw, int iters) {
    uint32_t h = 2166136261u;
    for (const char *p = salt; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    for (const char *p = pw; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    for (int i = 1; i < iters; i++) {
        uint32_t v = h;
        h = 2166136261u;
        for (int k = 0; k < 4; k++) { h ^= (v >> (8 * k)) & 255u; h *= 16777619u; }
        for (const char *p = salt; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
    }
    return h;
}

int dict_attack(uint32_t target, const char *salt, int iters, const char *const *words, int n, long *work) {
    *work = 0;
    for (int i = 0; i < n; i++) {
        *work += iters;
        if (toy_hash(salt, words[i], iters) == target) return i;
    }
    return -1;
}

int shared_hashes(const uint32_t *hashes, int n) {
    int c = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) if (i != j && hashes[i] == hashes[j]) { c++; break; }
    }
    return c;
}

long iterations_needed(double rate, long words, double seconds) {
    long k = (long)ceil(seconds * rate / (double)words);
    while (k > 1 && (double)words * (double)(k - 1) / rate >= seconds) k--;
    while ((double)words * (double)k / rate < seconds) k++;
    return k < 1 ? 1 : k;
}

int main(void) {
    const char *words[] = { "lantern", "orchid", "willow" };
    long work;
    uint32_t t = toy_hash("s1", "orchid", 100);
    printf("%d work=%ld\n", dict_attack(t, "s1", 100, words, 3, &work), work);
    return 0;
}
