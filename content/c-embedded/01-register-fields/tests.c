typedef struct { bus_t bus; uint32_t regs[4]; int reads, writes; } M;
static uint32_t m_read(bus_t *b, uint32_t off) { M *m = (M *)b; m->reads++; return m->regs[off / 4]; }
static void m_write(bus_t *b, uint32_t off, uint32_t v) { M *m = (M *)b; m->writes++; m->regs[off / 4] = v; }
static uint32_t m_now(bus_t *b) { (void)b; return 0; }
static void m_init(M *m, uint32_t reg1) {
    memset(m, 0, sizeof *m);
    m->bus.read = m_read; m->bus.write = m_write; m->bus.now = m_now;
    m->regs[1] = reg1;
}

// Independent reference: one bit at a time.
static uint32_t ref_set(uint32_t old, unsigned shift, unsigned width, uint32_t val) {
    for (unsigned i = 0; i < width; i++) {
        uint32_t bit = (val >> i) & 1u;
        old = (old & ~(1u << (shift + i))) | (bit << (shift + i));
    }
    return old;
}
static uint32_t ref_get(uint32_t reg, unsigned shift, unsigned width) {
    uint32_t v = 0;
    for (unsigned i = 0; i < width; i++) v |= ((reg >> (shift + i)) & 1u) << i;
    return v;
}

static const struct { unsigned shift, width; } F[] = {
    {0,1},{3,1},{31,1},{0,2},{4,2},{1,2},{4,8},{8,8},{24,8},{16,16},{5,12},{0,16},{20,12},{0,31},{1,31},{0,32}
};
#define NF ((int)(sizeof F / sizeof F[0]))
static const uint32_t OLD[] = { 0u, 0xFFFFFFFFu, 0xA5A5A5A5u, 0x12345678u, 0x80000001u };
#define NOLD ((int)(sizeof OLD / sizeof OLD[0]))
static const uint32_t VAL[] = { 0u, 1u, 0x5A5A5A5Au, 0xFFFFFFFFu, 2u, 0x80000000u };
#define NVAL ((int)(sizeof VAL / sizeof VAL[0]))

static char why[160];

static int all_get(void) {
    for (int f = 0; f < NF; f++) for (int o = 0; o < NOLD; o++) {
        M m; m_init(&m, OLD[o]);
        uint32_t got = field_get(&m.bus, 4, F[f].shift, F[f].width);
        uint32_t want = ref_get(OLD[o], F[f].shift, F[f].width);
        if (got != want) { snprintf(why, sizeof why, "shift %u width %u of 0x%08X gave 0x%X, expected 0x%X", F[f].shift, F[f].width, OLD[o], got, want); return 0; }
    }
    return 1;
}
static int get_counts(void) {
    for (int f = 0; f < NF; f++) {
        M m; m_init(&m, 0xDEADBEEFu);
        field_get(&m.bus, 4, F[f].shift, F[f].width);
        if (m.reads != 1 || m.writes != 0) { snprintf(why, sizeof why, "%d reads and %d writes, expected 1 and 0", m.reads, m.writes); return 0; }
    }
    return 1;
}
// fits_only: use only values that fit in the field; otherwise only wider ones.
static int set_cases(int fits) {
    for (int f = 0; f < NF; f++) for (int o = 0; o < NOLD; o++) for (int v = 0; v < NVAL; v++) {
        uint32_t m_ = F[f].width >= 32 ? 0xFFFFFFFFu : (1u << F[f].width) - 1u;
        int fit = (VAL[v] & ~m_) == 0;
        if (fit != fits) continue;
        M m; m_init(&m, OLD[o]);
        field_set(&m.bus, 4, F[f].shift, F[f].width, VAL[v]);
        uint32_t want = ref_set(OLD[o], F[f].shift, F[f].width, VAL[v]);
        if (m.regs[1] != want) { snprintf(why, sizeof why, "set shift %u width %u val 0x%X on 0x%08X gave 0x%08X, expected 0x%08X", F[f].shift, F[f].width, VAL[v], OLD[o], m.regs[1], want); return 0; }
        if (m.regs[0] || m.regs[2] || m.regs[3]) { snprintf(why, sizeof why, "another register was modified"); return 0; }
    }
    return 1;
}
static int set_counts(void) {
    for (int f = 0; f < NF; f++) {
        M m; m_init(&m, 0xCAFEF00Du);
        field_set(&m.bus, 4, F[f].shift, F[f].width, 1);
        if (m.reads != 1 || m.writes != 1) { snprintf(why, sizeof why, "%d reads and %d writes, expected 1 and 1", m.reads, m.writes); return 0; }
    }
    return 1;
}

int main(void) {
    why[0] = 0;
    CHECK("field_get extracts every field", all_get(), "%s", why);
    CHECK("field_get reads once and never writes", get_counts(), "%s", why);
    CHECK("field_set keeps neighbouring bits", set_cases(1), "%s", why);
    CHECK("field_set truncates a value wider than the field", set_cases(0), "%s", why);
    CHECK("field_set is one read and one write", set_counts(), "%s", why);

    M m; m_init(&m, 0x00000FF1u);
    field_set(&m.bus, 4, 1, 2, 2);
    CHECK("the lesson example gives 0x00000FF5", m.regs[1] == 0x00000FF5u, "got 0x%08X", m.regs[1]);
    m_init(&m, 0u);
    field_set(&m.bus, 4, 0, 32, 0xFFFFFFFFu);
    CHECK("a 32-bit field replaces the whole register", m.regs[1] == 0xFFFFFFFFu, "got 0x%08X", m.regs[1]);
    return 0;
}
