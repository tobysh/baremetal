// ---- reference: parent pointers and states kept in separate arrays, ancestry by transitive closure ----
typedef struct { int st[NPROC], par[NPROC], code[NPROC]; } rt;
static void r_init(rt *r) { memset(r, 0, sizeof *r); r->st[1] = 1; }
static int r_fork(rt *r, int par) {
    if (par < 1 || par > 15 || r->st[par] != 1) return -1;
    int pid = 2; while (pid < NPROC && r->st[pid] != 0) pid++;
    if (pid == NPROC) return -1;
    r->st[pid] = 1; r->par[pid] = par; r->code[pid] = 0; return pid;
}
static int r_exit(rt *r, int pid, int code) {
    if (pid < 2 || pid > 15 || r->st[pid] != 1) return -1;
    r->st[pid] = 2; r->code[pid] = code;
    for (int q = 2; q < NPROC; q++) if (r->st[q] && r->par[q] == pid) r->par[q] = 1;
    return 0;
}
static int r_wait(rt *r, int par, int *code) {
    if (par < 1 || par > 15 || r->st[par] != 1) return -1;
    int have = 0;
    for (int q = 2; q < NPROC; q++) if (r->st[q] && r->par[q] == par) have = 1;
    for (int q = 2; q < NPROC; q++) if (r->st[q] == 2 && r->par[q] == par) { *code = r->code[q]; r->st[q] = 0; r->par[q] = 0; r->code[q] = 0; return q; }
    return have ? 0 : -1;
}
static int r_desc(const rt *r, int pid) {
    if (pid < 1 || pid > 15 || !r->st[pid]) return -1;
    int below[NPROC] = {0}, changed = 1;
    below[pid] = 1;
    while (changed) {
        changed = 0;
        for (int q = 2; q < NPROC; q++) if (r->st[q] && !below[q] && below[r->par[q]]) { below[q] = 1; changed = 1; }
    }
    int n = 0; for (int q = 1; q < NPROC; q++) if (below[q] && q != pid) n++;
    return n;
}
static const char *same(const ptable *t, const rt *r) {
    static char w[120];
    for (int i = 1; i < NPROC; i++) {
        if (t->p[i].state != r->st[i]) { snprintf(w, sizeof w, "pid %d state %d, expected %d", i, t->p[i].state, r->st[i]); return w; }
        if (r->st[i] && i > 1 && t->p[i].ppid != r->par[i]) { snprintf(w, sizeof w, "pid %d parent %d, expected %d", i, t->p[i].ppid, r->par[i]); return w; }
        if (r->st[i] == 2 && t->p[i].status != r->code[i]) { snprintf(w, sizeof w, "pid %d status %d, expected %d", i, t->p[i].status, r->code[i]); return w; }
    }
    return NULL;
}
static unsigned t_rng = 1;
static unsigned t_next(void) { t_rng = t_rng * 1103515245u + 12345u; return (t_rng >> 16) & 0x7fff; }

int main(void) {
    ptable t; pt_init(&t);
    int a = pt_fork(&t, 1), b = pt_fork(&t, a), c = pt_fork(&t, b), d = pt_fork(&t, b);
    CHECK("fork hands out the lowest free pids", a == 2 && b == 3 && c == 4 && d == 5, "got %d %d %d %d", a, b, c, d);
    CHECK("descendants counts the whole subtree", pt_descendants(&t, a) == 3 && pt_descendants(&t, b) == 2 && pt_descendants(&t, c) == 0 && pt_descendants(&t, 1) == 4, "%d %d %d %d", pt_descendants(&t, a), pt_descendants(&t, b), pt_descendants(&t, c), pt_descendants(&t, 1));
    CHECK("init cannot exit", pt_exit(&t, 1, 0) == -1 && t.p[1].state == P_RUN, "init exited");
    CHECK("exit makes a zombie that still has a slot", pt_exit(&t, c, 7) == 0 && t.p[c].state == P_ZOMBIE && t.p[c].status == 7 && t.p[c].ppid == b, "state %d", t.p[c].state);
    CHECK("exit twice fails", pt_exit(&t, c, 8) == -1 && t.p[c].status == 7, "second exit changed the status");
    CHECK("a zombie still counts as a descendant", pt_descendants(&t, b) == 2, "got %d", pt_descendants(&t, b));
    CHECK("a zombie cannot fork", pt_fork(&t, c) == -1, "forked from a zombie");
    int st = 0;
    CHECK("wait reaps the zombie child with its status", pt_wait(&t, b, &st) == c && st == 7 && t.p[c].state == P_FREE, "reaped wrong or status %d", st);
    CHECK("wait with live children but no zombies returns 0", pt_wait(&t, b, &st) == 0, "wrong result");
    CHECK("wait with no children returns -1", pt_wait(&t, d, &st) == -1 && pt_wait(&t, 9, &st) == -1, "wrong result");
    pt_exit(&t, b, 1);
    CHECK("children of an exiting process go to init", t.p[d].ppid == 1 && t.p[b].state == P_ZOMBIE, "d's parent is %d", t.p[d].ppid);
    pt_exit(&t, d, 2);
    CHECK("init reaps orphans that have exited", pt_wait(&t, 1, &st) == d && st == 2, "wait returned the wrong pid or status %d", st);
    CHECK("the lowest zombie is reaped first", pt_wait(&t, a, &st) == b && st == 1, "wrong pid or status %d", st);
    int e = pt_fork(&t, a);
    CHECK("freed pids are reused lowest first", e == 3, "got %d", e);
    ptable full; pt_init(&full);
    int n = 0; while (pt_fork(&full, 1) > 0) n++;
    CHECK("the table holds 14 children of init", n == 14, "forked %d", n);

    const char *bad = NULL; static char msg[200];
    for (unsigned seed = 1; seed <= 10 && !bad; seed++) {
        ptable x; rt r; pt_init(&x); r_init(&r); t_rng = seed * 7777;
        for (int step = 0; step < 400 && !bad; step++) {
            int op = t_next() % 5, pid = 1 + t_next() % 15, code = t_next() % 100, g, w, g2 = -7, w2 = -7;
            if (t_next() % 11 == 0) pid = (int)(t_next() % 18) - 1;
            if (op <= 1) { g = pt_fork(&x, pid); w = r_fork(&r, pid); }
            else if (op == 2) { g = pt_exit(&x, pid, code); w = r_exit(&r, pid, code); }
            else if (op == 3) { g = pt_wait(&x, pid, &g2); w = r_wait(&r, pid, &w2); if (w <= 0) g2 = w2 = 0; }
            else { g = pt_descendants(&x, pid); w = r_desc(&r, pid); }
            const char *s;
            if (g != w || g2 != w2) { snprintf(msg, sizeof msg, "seed %u step %d op %d pid %d: returned %d (status %d), expected %d (status %d)", seed, step, op, pid, g, g2, w, w2); bad = msg; }
            else if ((s = same(&x, &r)) != NULL) { snprintf(msg, sizeof msg, "seed %u step %d op %d pid %d: %s", seed, step, op, pid, s); bad = msg; }
        }
    }
    CHECK("4000 random fork/exit/wait/descendants steps match the reference", bad == NULL, "%s", bad ? bad : "");
    return 0;
}
