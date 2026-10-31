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
    const char *why = NULL; static char msg[200];
    for (unsigned seed = 1; seed <= 8 && !why; seed++) {
        t_rng = seed * 31337; sys_init(&s); fdt_init(&t); m_init(&m);
        for (int step = 0; step < 400 && !why; step++) {
            int op = t_next() % 7, a = (int)(t_next() % (NFD + 2)) - 1, b = (int)(t_next() % (NFD + 2)) - 1, fl = t_next() & 1;
            int got = 0, want = 0, n;
            if (op <= 1) { int ino = 1 + t_next() % 40; got = fd_open(&s, &t, ino); want = m_open(&m, 0, ino); }
            else if (op == 2) { got = fd_close(&s, &t, a); want = m_close(&m, 0, a); }
            else if (op == 3) { got = fd_dup(&s, &t, a); want = m_dup(&m, 0, a); }
            else if (op == 4) { got = fd_dup3(&s, &t, a, b, fl); want = (a == b) ? -1 : m_dup2(&m, 0, a, b); if (want >= 0 && fl) want |= 0x100; }
            else if (op == 5) { got = fd_closefrom(&s, &t, a); want = 0; for (int i = a < 0 ? 0 : a; i < NFD; i++) if (m_close(&m, 0, i) == 0) want++; }
            else { got = fd_dup2(&s, &t, a, b); want = m_dup2(&m, 0, a, b); }
            const char *d;
            if (got != want) { snprintf(msg, sizeof msg, "seed %u step %d op %d (%d, %d, flag %d): returned %d, expected %d", seed, step, op, a, b, fl, got, want); why = msg; }
            else if ((d = m_diff(&m, &s, &t, 1)) != NULL) { snprintf(msg, sizeof msg, "seed %u step %d op %d: %s", seed, step, op, d); why = msg; }
            (void)n;
        }
    }
    CHECK("dup3 and closefrom match the reference over 3200 random steps", why == NULL, "%s", why ? why : "");
    sys_init(&s); fdt_init(&t);
    int x = fd_open(&s, &t, 1);
    CHECK("dup3 refuses oldfd == newfd, unlike dup2", fd_dup3(&s, &t, x, x, 0) == -1 && fd_dup2(&s, &t, x, x) == x, "dup3 accepted equal numbers");
    CHECK("the flag is reported", fd_dup3(&s, &t, x, 3, 1) == (3 | 0x100) && fd_dup3(&s, &t, x, 4, 0) == 4, "wrong return value");
    return 0;
}
