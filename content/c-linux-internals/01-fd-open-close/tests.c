// ---- independent reference: counts references instead of storing a refcount ----
#define MT 3
typedef struct {
    int d[MT][NFD];                 // description id per fd, -1 when closed
    int ino[NOFD]; long off[NOFD]; int live[NOFD];
} model;

static void m_init(model *m) {
    memset(m, 0, sizeof *m);
    for (int t = 0; t < MT; t++) for (int i = 0; i < NFD; i++) m->d[t][i] = -1;
}
static int m_refs(const model *m, int dsc) {
    int n = 0;
    for (int t = 0; t < MT; t++) for (int i = 0; i < NFD; i++) if (m->d[t][i] == dsc) n++;
    return n;
}
static void m_reap(model *m) {
    for (int j = 0; j < NOFD; j++)
        if (m->live[j] && m_refs(m, j) == 0) { m->live[j] = 0; m->ino[j] = 0; m->off[j] = 0; }
}
static int m_okfd(const model *m, int t, int fd) { return fd >= 0 && fd < NFD && m->d[t][fd] >= 0; }
static int m_open(model *m, int t, int ino) {
    int fd = -1, j = -1;
    for (int i = NFD - 1; i >= 0; i--) if (m->d[t][i] < 0) fd = i;
    for (int k = NOFD - 1; k >= 0; k--) if (!m->live[k]) j = k;
    if (fd < 0 || j < 0) return -1;
    m->live[j] = 1; m->ino[j] = ino; m->off[j] = 0; m->d[t][fd] = j;
    return fd;
}
static int m_close(model *m, int t, int fd) {
    if (!m_okfd(m, t, fd)) return -1;
    m->d[t][fd] = -1; m_reap(m);
    return 0;
}
static long m_adv(model *m, int t, int fd, long n) {
    if (!m_okfd(m, t, fd) || n < 0) return -1;
    m->off[m->d[t][fd]] += n;
    return m->off[m->d[t][fd]];
}
static int m_dup(model *m, int t, int fd) {
    if (!m_okfd(m, t, fd)) return -1;
    for (int i = 0; i < NFD; i++) if (m->d[t][i] < 0) { m->d[t][i] = m->d[t][fd]; return i; }
    return -1;
}
static int m_dup2(model *m, int t, int o, int n) {
    if (!m_okfd(m, t, o) || n < 0 || n >= NFD) return -1;
    if (o == n) return n;
    m->d[t][n] = m->d[t][o]; m_reap(m);
    return n;
}

// Compare the learner's tables with the model. Returns NULL when they agree, else a reason.
static const char *m_diff(const model *m, const sysfiles *s, const fdtable *t, int nt) {
    static char why[160];
    int live = 0, used = 0;
    for (int j = 0; j < NOFD; j++) { if (m->live[j]) live++; if (s->files[j].refs > 0) used++; }
    if (live != used) { snprintf(why, sizeof why, "%d open file descriptions in use, expected %d", used, live); return why; }
    for (int a = 0; a < nt; a++) for (int i = 0; i < NFD; i++) {
        int open_m = m->d[a][i] >= 0, open_l = t[a].slot[i] >= 0;
        if (open_m != open_l) { snprintf(why, sizeof why, "table %d fd %d is %s, expected %s", a, i, open_l ? "open" : "closed", open_m ? "open" : "closed"); return why; }
        if (!open_m) continue;
        int f = t[a].slot[i];
        if (f >= NOFD) { snprintf(why, sizeof why, "table %d fd %d points at file %d", a, i, f); return why; }
        const ofd *o = &s->files[f];
        if (o->ino != m->ino[m->d[a][i]] || o->off != m->off[m->d[a][i]]) { snprintf(why, sizeof why, "table %d fd %d has ino %d offset %ld, expected ino %d offset %ld", a, i, o->ino, o->off, m->ino[m->d[a][i]], m->off[m->d[a][i]]); return why; }
        if (o->refs != m_refs(m, m->d[a][i])) { snprintf(why, sizeof why, "table %d fd %d: refs is %d, expected %d", a, i, o->refs, m_refs(m, m->d[a][i])); return why; }
        for (int b = 0; b < nt; b++) for (int k = 0; k < NFD; k++) {
            if (m->d[b][k] < 0) continue;
            if ((m->d[a][i] == m->d[b][k]) != (t[a].slot[i] == t[b].slot[k])) { snprintf(why, sizeof why, "sharing of table %d fd %d and table %d fd %d is wrong", a, i, b, k); return why; }
        }
    }
    return NULL;
}

static unsigned t_rng = 1;
static unsigned t_next(void) { t_rng = t_rng * 1103515245u + 12345u; return (t_rng >> 16) & 0x7fff; }


int main(void) {
    sysfiles s; fdtable t; model m;
    sys_init(&s); fdt_init(&t); m_init(&m);
    int a = fd_open(&s, &t, 7), b = fd_open(&s, &t, 9);
    CHECK("open hands out 0 then 1", a == 0 && b == 1, "got %d and %d", a, b);
    CHECK("a new file starts at offset 0", s.files[t.slot[0]].off == 0 && s.files[t.slot[0]].ino == 7, "ino %d offset %ld", s.files[t.slot[0]].ino, s.files[t.slot[0]].off);
    long r = fd_advance(&s, &t, 0, 40); r = fd_advance(&s, &t, 0, 2);
    CHECK("advance accumulates", r == 42, "returned %ld, expected 42", r);
    CHECK("advance on fd 1 does not move fd 0", fd_advance(&s, &t, 1, 5) == 5 && s.files[t.slot[0]].off == 42, "descriptors share an offset by mistake");
    CHECK("closing fd 0 succeeds once", fd_close(&s, &t, 0) == 0 && fd_close(&s, &t, 0) == -1, "double close was not rejected");
    int c = fd_open(&s, &t, 11);
    CHECK("the lowest free number is reused", c == 0, "got %d, expected 0", c);
    CHECK("a reused slot starts fresh", s.files[t.slot[0]].off == 0 && s.files[t.slot[0]].ino == 11, "offset %ld ino %d", s.files[t.slot[0]].off, s.files[t.slot[0]].ino);
    CHECK("bad descriptors are rejected", fd_close(&s, &t, -1) == -1 && fd_close(&s, &t, NFD) == -1 && fd_close(&s, &t, 5) == -1 && fd_advance(&s, &t, 6, 1) == -1 && fd_advance(&s, &t, 0, -1) == -1, "a bad argument was accepted");

    // Fill the table: the ninth open fails and changes nothing.
    sys_init(&s); fdt_init(&t);
    int last = 0;
    for (int i = 0; i < NFD; i++) last = fd_open(&s, &t, 100 + i);
    int over = fd_open(&s, &t, 999);
    CHECK("a full table returns -1", last == NFD - 1 && over == -1, "last %d, overflow %d", last, over);
    int ok = 1; for (int i = 0; i < NFD; i++) if (s.files[t.slot[i]].ino != 100 + i) ok = 0;
    CHECK("a failed open changes nothing", ok, "an existing entry was overwritten");

    // Failure schedules: random operation sequences against the counting model.
    const char *bad = NULL; int at = -1;
    for (unsigned seed = 1; seed <= 6 && !bad; seed++) {
        t_rng = seed * 7919; sys_init(&s); fdt_init(&t); m_init(&m);
        for (int step = 0; step < 300 && !bad; step++) {
            int op = t_next() % 4, fd = (int)(t_next() % (NFD + 2)) - 1, ino = 1 + t_next() % 50;
            long n = (long)(t_next() % 12) - 1;
            int got, want;
            if (op == 0 || op == 3) { got = fd_open(&s, &t, ino); want = m_open(&m, 0, ino); }
            else if (op == 1) { got = fd_close(&s, &t, fd); want = m_close(&m, 0, fd); }
            else { got = (int)fd_advance(&s, &t, fd, n); want = (int)m_adv(&m, 0, fd, n); }
            static char msg[200];
            if (got != want) { snprintf(msg, sizeof msg, "seed %d step %d op %d (fd %d): returned %d, expected %d", seed, step, op, fd, got, want); bad = msg; }
            else if ((bad = m_diff(&m, &s, &t, 1)) != NULL) { snprintf(msg, sizeof msg, "seed %d step %d op %d: %s", seed, step, op, bad); bad = msg; }
            at = step;
        }
    }
    CHECK("1800 random open/close/advance steps match the reference", bad == NULL, "%s", bad ? bad : "");
    (void)at;
    return 0;
}
