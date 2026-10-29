static size_t bounded_len(const char *s,size_t cap){size_t n=0;while(n<cap && s[n])n++;return n;}
/* Guard bytes: 16 before the buffer and everything after `cap` is pre-filled
   with a pattern. If the learner's code writes even one byte out of bounds,
   the pattern changes. No crash is needed to notice. */
#define GPOST 512
typedef struct { unsigned char pre[16]; char data[64]; unsigned char post[GPOST]; } Guard;
static void g_init(Guard *g) {
    memset(g->pre, 0xA5, sizeof g->pre);
    memset(g->data, 0x5A, sizeof g->data);
    memset(g->post, 0xA5, sizeof g->post);
}
static int g_ok(const Guard *g, size_t cap) {
    for (size_t i = 0; i < sizeof g->pre; i++) if (g->pre[i] != 0xA5) return 0;
    for (size_t i = cap; i < sizeof g->data; i++) if ((unsigned char)g->data[i] != 0x5A) return 0;
    for (size_t i = 0; i < GPOST; i++) if (g->post[i] != 0xA5) return 0;
    return 1;
}

static char t_long[400];

static void t_case(const char *label, size_t cap, const char *src) {
    char name[160];
    Guard g;
    g_init(&g);
    copy_label(g.data, cap, src);
    snprintf(name, sizeof name, "%s: nothing written outside the %zu-byte buffer", label, cap);
    CHECK(name, g_ok(&g, cap), "a guard byte was overwritten (you wrote past dst[%zu])", cap ? cap - 1 : (size_t)0);
    if (cap == 0) return;
    size_t want = strlen(src);
    if (want > cap - 1) want = cap - 1;
    snprintf(name, sizeof name, "%s: result is the first %zu characters, terminated", label, want);
    CHECK(name, bounded_len(g.data, sizeof g.data) == want && strncmp(g.data, src, want) == 0 && g.data[want] == '\0',
          "got \"%.40s\" (length %zu)", g.data, bounded_len(g.data, sizeof g.data));
}

int main(void) {
    for (int i = 0; i < 399; i++) t_long[i] = (char)('a' + i % 26);
    t_long[399] = '\0';

    t_case("empty string", 8, "");
    t_case("short string", 8, "abc");
    t_case("one short of full", 8, "abcdefg");
    t_case("exactly full", 8, "abcdefgh");
    t_case("one too long", 8, "abcdefghi");
    t_case("far too long", 8, t_long);
    t_case("cap of 1", 1, "hello");
    t_case("cap of 2", 2, "hello");
    t_case("cap of 32 and a 399-char string", 32, t_long);
    t_case("cap of 17 and a 16-char string", 17, "0123456789abcdef");

    {
        Guard g;
        g_init(&g);
        copy_label(g.data, 0, "hello");
        CHECK("cap of 0 writes nothing", g_ok(&g, 0), "a byte was written although cap was 0");
    }
    {
        Guard g;
        g_init(&g);
        copy_label(g.data, 10, NULL);
        CHECK("NULL src gives an empty string", g_ok(&g, 10) && g.data[0] == '\0', "dst[0] was %d", g.data[0]);
    }
    copy_label(NULL, 10, "hello");
    CHECK("NULL dst is ignored", 1, "");
    return 0;
}
