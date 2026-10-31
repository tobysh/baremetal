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


static const char *t_step(model *m, sysfiles *s, fdtable *t, unsigned seed, int steps, int op_limit) {
    static char msg[220];
    t_rng = seed * 104729; sys_init(s); fdt_init(t); m_init(m);
    for (int step = 0; step < steps; step++) {
        int op = t_next() % op_limit, fd = (int)(t_next() % (NFD + 2)) - 1, fd2 = (int)(t_next() % (NFD + 2)) - 1;
        int ino = 1 + t_next() % 50; long n = (long)(t_next() % 12) - 1;
        int got, want;
        if (op <= 1) { got = fd_open(s, t, ino); want = m_open(m, 0, ino); }
        else if (op == 2) { got = fd_close(s, t, fd); want = m_close(m, 0, fd); }
        else if (op == 3) { got = (int)fd_advance(s, t, fd, n); want = (int)m_adv(m, 0, fd, n); }
        else if (op <= 5) { got = fd_dup(s, t, fd); want = m_dup(m, 0, fd); }
        else { got = fd_dup2(s, t, fd, fd2); want = m_dup2(m, 0, fd, fd2); }
        const char *d;
        if (got != want) { snprintf(msg, sizeof msg, "seed %u step %d op %d (fd %d, %d): returned %d, expected %d", seed, step, op, fd, fd2, got, want); return msg; }
        if ((d = m_diff(m, s, t, 1)) != NULL) { snprintf(msg, sizeof msg, "seed %u step %d op %d (fd %d, %d): %s", seed, step, op, fd, fd2, d); return msg; }
    }
    return NULL;
}

int main(void) {
    sysfiles s; fdtable t; model m;
    sys_init(&s); fdt_init(&t);
    int a = fd_open(&s, &t, 7);
    int b = fd_dup(&s, &t, a);
    CHECK("dup uses the lowest free number", b == 1, "got %d, expected 1", b);
    CHECK("dup shares one description", t.slot[a] == t.slot[b] && s.files[t.slot[a]].refs == 2, "slots %d and %d, refs %d", t.slot[a], t.slot[b], s.files[t.slot[a]].refs);
    fd_advance(&s, &t, a, 30);
    CHECK("the offset is shared", s.files[t.slot[b]].off == 30, "offset through dup is %ld, expected 30", s.files[t.slot[b]].off);
    fd_close(&s, &t, a);
    CHECK("the description survives while one name remains", s.files[t.slot[b]].refs == 1 && s.files[t.slot[b]].off == 30, "refs %d offset %ld", s.files[t.slot[b]].refs, s.files[t.slot[b]].off);
    int c = fd_open(&s, &t, 7);
    fd_advance(&s, &t, c, 5);
    CHECK("opening the same file twice does not share an offset", s.files[t.slot[b]].off == 30 && t.slot[c] != t.slot[b], "two opens share one description");

    sys_init(&s); fdt_init(&t);
    fd_open(&s, &t, 100); fd_open(&s, &t, 101); int f = fd_open(&s, &t, 555);
    int r = fd_dup2(&s, &t, f, 1);
    CHECK("dup2 returns newfd and replaces it", r == 1 && s.files[t.slot[1]].ino == 555, "returned %d, fd 1 has ino %d", r, s.files[t.slot[1]].ino);
    int freed = 0; for (int j = 0; j < NOFD; j++) if (s.files[j].refs > 0 && s.files[j].ino == 101) freed++;
    CHECK("the replaced description is freed when it was the last name", freed == 0, "the old stdout description is still allocated");
    fd_close(&s, &t, f);
    CHECK("after closing the spare, fd 1 keeps the file", s.files[t.slot[1]].ino == 555 && s.files[t.slot[1]].refs == 1, "refs %d", s.files[t.slot[1]].refs);

    sys_init(&s); fdt_init(&t);
    int x = fd_open(&s, &t, 3);
    CHECK("dup2 with equal numbers is a no-op", fd_dup2(&s, &t, x, x) == x && s.files[t.slot[x]].refs == 1, "refs %d", s.files[t.slot[x]].refs);
    CHECK("dup2 with equal but closed numbers fails", fd_dup2(&s, &t, 4, 4) == -1, "accepted a closed descriptor");
    CHECK("dup2 rejects bad numbers", fd_dup2(&s, &t, x, NFD) == -1 && fd_dup2(&s, &t, x, -1) == -1 && fd_dup2(&s, &t, 5, 2) == -1 && fd_dup(&s, &t, -1) == -1, "a bad argument was accepted");
    CHECK("a failed dup2 closes nothing", s.files[t.slot[x]].refs == 1, "refs %d", s.files[t.slot[x]].refs);

    sys_init(&s); fdt_init(&t);
    for (int i = 0; i < NFD; i++) fd_open(&s, &t, i);
    CHECK("dup in a full table fails and changes nothing", fd_dup(&s, &t, 0) == -1 && s.files[t.slot[0]].refs == 1, "refs %d", s.files[t.slot[0]].refs);

    const char *bad = NULL;
    for (unsigned seed = 1; seed <= 8 && !bad; seed++) bad = t_step(&m, &s, &t, seed, 400, 7);
    CHECK("3200 random steps with dup and dup2 match the reference", bad == NULL, "%s", bad ? bad : "");
    return 0;
}
