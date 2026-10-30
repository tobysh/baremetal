#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXW 4096

enum { ST_TOKENIZE, ST_COUNT, ST_RENDER, NSTAGES };

// Operation counters, one row per stage. Do not edit this block:
// the counters are the instrument, not the thing being fixed.
typedef struct { long cmp, alloc, copied; } stats_t;
stats_t stats[NSTAGES];

static void stats_reset(void) { memset(stats, 0, sizeof stats); }

static int same(int st, const char *a, const char *b) {
    stats[st].cmp++;
    return strcmp(a, b) == 0;
}

static char *dup_word(int st, const char *s, size_t n) {
    stats[st].alloc++;
    stats[st].copied += (long)n + 1;
    char *p = malloc(n + 1);
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

// Stage 1: split text on spaces into freshly allocated words.
static int tokenize(const char *text, char **words, int max) {
    int n = 0;
    const char *p = text;
    while (*p && n < max) {
        while (*p == ' ') p++;
        if (!*p) break;
        const char *q = p;
        while (*q && *q != ' ') q++;
        words[n++] = dup_word(ST_TOKENIZE, p, (size_t)(q - p));
        p = q;
    }
    return n;
}

// Stage 2: collect the distinct words in order of first appearance and
// count them. Returns the number of distinct words; uniq[i] points into words.
static int count_words(char **words, int n, char **uniq, int *counts) {
    enum { SLOTS = 8192 };
    static int head[SLOTS];      // slot -> index into uniq + 1, 0 = empty
    static int next[MAXW];       // chain of uniq indices in the same slot
    memset(head, 0, sizeof head);
    int u = 0;
    for (int i = 0; i < n; i++) {
        unsigned h = 5381;
        for (const char *c = words[i]; *c; c++) h = h * 33 + (unsigned char)*c;
        h %= SLOTS;
        int found = -1;
        for (int j = head[h] - 1; j >= 0; j = next[j]) {
            if (same(ST_COUNT, words[i], uniq[j])) { found = j; break; }
        }
        if (found < 0) {
            uniq[u] = words[i]; counts[u] = 1;
            next[u] = head[h] - 1;
            head[h] = u + 1;
            u++;
        } else counts[found]++;
    }
    return u;
}

// Stage 3: write "word=count" lines into out. Returns the length written.
static size_t render(char **uniq, const int *counts, int u, char *out, size_t cap) {
    size_t len = 0;
    for (int i = 0; i < u; i++) {
        int w = snprintf(out + len, cap - len, "%s=%d\n", uniq[i], counts[i]);
        stats[ST_RENDER].copied += w;
        len += (size_t)w;
    }
    return len;
}

// The whole pipeline. Returns the number of distinct words.
int word_freq(const char *text, char *out, size_t cap) {
    char *words[MAXW], *uniq[MAXW];
    int counts[MAXW];
    int n = tokenize(text, words, MAXW);
    int u = count_words(words, n, uniq, counts);
    render(uniq, counts, u, out, cap);
    for (int i = 0; i < n; i++) free(words[i]);
    return u;
}

static void profile(void) {
    static const char *names[NSTAGES] = {"tokenize", "count", "render"};
    printf("%-9s %10s %8s %8s\n", "stage", "compares", "allocs", "copied");
    for (int s = 0; s < NSTAGES; s++) {
        printf("%-9s %10ld %8ld %8ld\n", names[s], stats[s].cmp, stats[s].alloc, stats[s].copied);
    }
}

// Builds the workload from the lesson (2000 words, 400 distinct) and prints its profile.
int main(void) {
    static char text[1 << 16], out[1 << 16];
    size_t len = 0;
    unsigned x = 12345;
    for (int i = 0; i < 2000; i++) {
        x = x * 1103515245u + 12345u;
        len += (size_t)sprintf(text + len, "w%u ", (x >> 8) % 400);
    }
    stats_reset();
    int u = word_freq(text, out, sizeof out);
    printf("%d distinct words\n", u);
    profile();
    return 0;
}
