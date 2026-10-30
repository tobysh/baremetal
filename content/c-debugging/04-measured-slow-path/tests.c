// Reference result, computed without the learner's code and without its counters.
static int ref_freq(const char *text, char *out) {
    static char seen[MAXW][40];
    static int cnt[MAXW];
    int u = 0, o = 0;
    const char *p = text;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        char w[40];
        int k = 0;
        while (*p && *p != ' ') w[k++] = *p++;
        w[k] = '\0';
        int j = 0;
        while (j < u && strcmp(seen[j], w) != 0) j++;
        if (j == u) { strcpy(seen[u], w); cnt[u] = 0; u++; }
        cnt[j]++;
    }
    for (int i = 0; i < u; i++) o += sprintf(out + o, "%s=%d\n", seen[i], cnt[i]);
    out[o] = '\0';
    return u;
}

static char text[1 << 16], got[1 << 16], want[1 << 16];

// kind 0: n tokens from `vocab` words (pseudo-random); kind 1: n distinct tokens.
static int make(int n, int vocab, unsigned seed, int pad) {
    size_t len = 0;
    unsigned x = seed;
    for (int i = 0; i < n; i++) {
        x = x * 1103515245u + 12345u;
        unsigned id = vocab ? (x >> 8) % (unsigned)vocab : (unsigned)i;
        len += (size_t)sprintf(text + len, "%*s%s%u", i ? 1 + (pad ? (int)(x >> 20) % 3 : 0) : 0, "", (id & 1) ? "tok" : "k", id);
    }
    return n;
}

static long total_cmp(void) { return stats[ST_COUNT].cmp; }

static int run(const char *label, int n, int *u_out, int *same_out) {
    (void)label;
    memset(got, 0, sizeof got);
    stats_reset();
    int u = word_freq(text, got, sizeof got);
    int ru = ref_freq(text, want);
    *u_out = u;
    *same_out = (u == ru && strcmp(got, want) == 0);
    return n;
}

int main(void) {
    int u, ok;

    text[0] = '\0';
    run("empty", 0, &u, &ok);
    CHECK("an empty text gives an empty result", ok && u == 0 && got[0] == '\0', "got %d words and '%s'", u, got);

    strcpy(text, "  one   two one  ");
    run("spaces", 3, &u, &ok);
    CHECK("repeated spaces are skipped", ok && u == 2 && strcmp(got, "one=2\ntwo=1\n") == 0, "got '%s'", got);

    strcpy(text, "x");
    run("single", 1, &u, &ok);
    CHECK("a single word counts once", ok && strcmp(got, "x=1\n") == 0, "got '%s'", got);

    make(300, 5, 7, 1);
    run("few", 300, &u, &ok);
    CHECK("few distinct words: output matches the reference", ok, "output differs from the reference (u=%d)", u);
    CHECK("few distinct words are all found", u >= 1 && u <= 5, "got %d distinct words, expected at most 5", u);

    make(2000, 400, 12345, 0);
    run("mixed", 2000, &u, &ok);
    CHECK("mixed workload: output matches the reference", ok, "output differs from the reference (u=%d)", u);
    CHECK("mixed workload: tokenize does one allocation per word", stats[ST_TOKENIZE].alloc == 2000, "tokenize made %ld allocations, expected 2000", stats[ST_TOKENIZE].alloc);
    CHECK("mixed workload: render copies each output byte once", stats[ST_RENDER].copied == (long)strlen(want), "render copied %ld bytes, expected %zu", stats[ST_RENDER].copied, strlen(want));
    CHECK("mixed workload: counting stays within 4 compares per word", total_cmp() <= 4L * 2000,
          "count stage made %ld compares, budget is %ld (profile it: that is the dominant stage)", total_cmp(), 4L * 2000);

    make(900, 0, 1, 1);
    run("distinct", 900, &u, &ok);
    CHECK("all-distinct words: output matches the reference", ok && u == 900, "u=%d, expected 900, output matches reference: %d", u, ok);
    CHECK("all-distinct words stay within budget", total_cmp() <= 4L * 900, "count stage made %ld compares, budget is %ld", total_cmp(), 4L * 900);

    make(1000, 1, 3, 0);
    run("one", 1000, &u, &ok);
    CHECK("one word repeated 1000 times", ok && u == 1, "u=%d, expected 1", u);

    make(3000, 1500, 99, 1);
    run("large", 3000, &u, &ok);
    CHECK("large workload: output matches the reference", ok, "output differs from the reference (u=%d)", u);
    CHECK("large workload stays within budget", total_cmp() <= 4L * 3000, "count stage made %ld compares, budget is %ld", total_cmp(), 4L * 3000);
    CHECK("tokenizing was not made worse", stats[ST_TOKENIZE].alloc <= 3000, "tokenize made %ld allocations for 3000 words", stats[ST_TOKENIZE].alloc);
    return 0;
}
