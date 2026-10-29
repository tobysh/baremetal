int main(void) {
    long h[256];
    for (int b = 0; b < 256; b++) h[b] = 999;
    parallel_histogram((const unsigned char *)"", 0, 4, h);
    int zero = 1;
    for (int b = 0; b < 256; b++) if (h[b] != 0) zero = 0;
    CHECK("an empty input gives an all-zero histogram", zero, "some counters were not reset");

    const unsigned char word[] = "mississippi";
    parallel_histogram(word, 11, 1, h);
    CHECK("one thread counts 'mississippi'", h['i'] == 4 && h['s'] == 4 && h['p'] == 2 && h['m'] == 1 && h['z'] == 0, "i=%ld s=%ld p=%ld m=%ld", h['i'], h['s'], h['p'], h['m']);
    parallel_histogram(word, 11, 8, h);
    CHECK("eight threads on eleven bytes give the same counts", h['i'] == 4 && h['s'] == 4 && h['p'] == 2 && h['m'] == 1, "i=%ld s=%ld p=%ld m=%ld", h['i'], h['s'], h['p'], h['m']);

    size_t n = 400000;
    unsigned char *big = malloc(n);
    long want[256] = {0};
    unsigned s = 12345;
    for (size_t i = 0; i < n; i++) {
        s = s * 1103515245u + 12345u;
        big[i] = (unsigned char)((s >> 16) & 0xFF);
        want[big[i]]++;
    }
    int ok = 1;
    int bad_k = 0;
    for (int k = 1; k <= 8; k++) {
        parallel_histogram(big, n, k, h);
        for (int b = 0; b < 256; b++) if (h[b] != want[b]) { ok = 0; bad_k = k; }
    }
    CHECK("400000 random bytes match a plain loop for 1 to 8 threads", ok, "wrong counts with %d threads", bad_k);

    long total = 0;
    for (int b = 0; b < 256; b++) total += h[b];
    CHECK("the counters add up to n", total == (long)n, "total %ld", total);

    int stable = 1;
    for (int rep = 0; rep < 20; rep++) {
        parallel_histogram(big, 20000 + (size_t)rep, 6, h);
        long t2 = 0;
        for (int b = 0; b < 256; b++) t2 += h[b];
        if (t2 != 20000 + rep) stable = 0;
    }
    CHECK("repeated runs on 6 threads always count every byte", stable, "a total was wrong");
    free(big);
    return 0;
}
