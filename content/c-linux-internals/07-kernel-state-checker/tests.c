// ---- test-side builders write the structures directly, so they do not depend on the learner's code ----
static unsigned t_rng = 1;
static unsigned t_next(void) { t_rng = t_rng * 1103515245u + 12345u; return (t_rng >> 16) & 0x7fff; }

static void use_file(kstate *k, int pid, int fd, int f, int ino) {
    k->fdt[pid].slot[fd] = f;
    if (k->files.files[f].refs == 0) { k->files.files[f].ino = ino; k->files.files[f].off = 0; }
    k->files.files[f].refs++;
}
static void set_proc(kstate *k, int pid, int state, int ppid) { k->pt.p[pid].state = state; k->pt.p[pid].ppid = ppid; }

static void base_state(kstate *k) {
    memset(k, 0, sizeof *k);
    for (int i = 0; i < NPROC; i++) for (int fd = 0; fd < NFD; fd++) k->fdt[i].slot[fd] = -1;
    set_proc(k, 1, P_RUN, 0); set_proc(k, 2, P_RUN, 1); set_proc(k, 3, P_RUN, 2); set_proc(k, 4, P_RUN, 2);
    set_proc(k, 5, P_ZOMBIE, 3); set_proc(k, 6, P_ZOMBIE, 1);
    k->pt.p[5].status = 1;
    use_file(k, 1, 0, 0, 10); use_file(k, 2, 0, 0, 10); use_file(k, 3, 0, 0, 10);
    use_file(k, 2, 1, 1, 11); use_file(k, 2, 2, 1, 11);
    use_file(k, 4, 0, 2, 12);
    k->sig[3].blocked = 1u << SG_USR1; k->sig[3].pending = 1u << SG_USR1;
    k->sig[2].disp[SG_CHLD] = D_CATCH; k->sig[2].pending = 1u << SG_CHLD;
    for (int i = 1; i < NPROC; i++) k->cr[i] = (cred){1000, 1000, 100, 100, {0}, 0};
}

// ---- independent reference for ks_check: per-concern passes, no shared helper code ----
static uint32_t r_check(const kstate *k) {
    uint32_t v = 0;
    for (int pid = 1; pid < NPROC; pid++) if (k->pt.p[pid].state == P_RUN)
        for (int fd = 0; fd < NFD; fd++) {
            int s = k->fdt[pid].slot[fd];
            if (s != -1 && (s < 0 || s >= NOFD || k->files.files[s].refs <= 0)) v |= 1u << 0;
        }
    for (int j = 0; j < NOFD; j++) {
        int n = 0;
        for (int pid = 1; pid < NPROC; pid++) for (int fd = 0; fd < NFD; fd++) if (k->fdt[pid].slot[fd] == j) n++;
        int r = k->files.files[j].refs;
        if (r < 0 || (r > 0 && r != n)) v |= 1u << 1;
    }
    for (int pid = 1; pid < NPROC; pid++) if (k->pt.p[pid].state != P_RUN)
        for (int fd = 0; fd < NFD; fd++) if (k->fdt[pid].slot[fd] != -1) v |= 1u << 2;
    for (int pid = 2; pid < NPROC; pid++) if (k->pt.p[pid].state != P_FREE) {
        int pp = k->pt.p[pid].ppid;
        if (pp < 1 || pp >= NPROC || k->pt.p[pp].state != P_RUN) v |= 1u << 3;
    }
    for (int pid = 2; pid < NPROC; pid++) if (k->pt.p[pid].state != P_FREE) {
        int seen[NPROC] = {0}, cur = pid, loop = 0;
        while (cur != 1 && cur >= 1 && cur < NPROC && k->pt.p[cur].state != P_FREE) {
            if (seen[cur]) { loop = 1; break; }
            seen[cur] = 1; cur = k->pt.p[cur].ppid;
        }
        if (loop) v |= 1u << 4;
    }
    for (int pid = 1; pid < NPROC; pid++) {
        int st = k->pt.p[pid].state;
        if (st != P_RUN && k->sig[pid].pending) v |= 1u << 5;
        if (st != P_FREE && ((k->sig[pid].blocked >> 9) & 1 || (k->sig[pid].blocked >> 19) & 1 || (k->sig[pid].blocked & 1))) v |= 1u << 6;
    }
    if (k->pt.p[1].state != P_RUN || k->pt.p[1].ppid != 0) v |= 1u << 7;
    return v;
}

typedef struct { const char *name; void (*apply)(kstate *); uint32_t expect; } mutator;
static void m_dangling(kstate *k) { k->fdt[3].slot[5] = 6; }
static void m_range(kstate *k) { k->fdt[4].slot[3] = 99; }
static void m_refs_hi(kstate *k) { k->files.files[2].refs = 2; }
static void m_refs_lo(kstate *k) { k->files.files[0].refs = 2; }
static void m_leak(kstate *k) { k->files.files[5].ino = 9; k->files.files[5].refs = 1; }
static void m_neg(kstate *k) { k->files.files[7].refs = -1; }
static void m_zfd(kstate *k) { k->fdt[5].slot[0] = 2; k->files.files[2].refs = 2; }
static void m_freefd(kstate *k) { k->fdt[9].slot[0] = 2; k->files.files[2].refs = 2; }
static void m_par_free(kstate *k) { k->pt.p[4].ppid = 9; }
static void m_par_zombie(kstate *k) { k->pt.p[4].ppid = 5; }
static void m_par_zero(kstate *k) { k->pt.p[4].ppid = 0; }
static void m_cycle(kstate *k) { k->pt.p[3].ppid = 4; k->pt.p[4].ppid = 3; }
static void m_self(kstate *k) { k->pt.p[2].ppid = 2; }
static void m_pend_zombie(kstate *k) { k->sig[5].pending = 1u << SG_USR1; }
static void m_pend_free(kstate *k) { k->sig[11].pending = 1u << SG_TERM; }
static void m_block_kill(kstate *k) { k->sig[2].blocked |= 1u << SG_KILL; }
static void m_block_stop_zombie(kstate *k) { k->sig[6].blocked |= 1u << SG_STOP; }
static void m_block_bit0(kstate *k) { k->sig[4].blocked |= 1u; }
static void m_init_parent(kstate *k) { k->pt.p[1].ppid = 2; }
static void m_none(kstate *k) { (void)k; }

static const mutator muts[] = {
    {"none", m_none, 0},
    {"a slot pointing at a free description", m_dangling, V_DANGLING},
    {"a slot index outside the table", m_range, V_DANGLING},
    {"refs too high", m_refs_hi, V_REFCOUNT},
    {"refs too low", m_refs_lo, V_REFCOUNT},
    {"a leaked description with no descriptor", m_leak, V_REFCOUNT},
    {"a negative refcount", m_neg, V_REFCOUNT},
    {"a zombie holding a descriptor", m_zfd, V_DEAD_FDS},
    {"a free slot holding a descriptor", m_freefd, V_DEAD_FDS},
    {"a parent that is free", m_par_free, V_PARENT},
    {"a parent that is a zombie", m_par_zombie, V_PARENT},
    {"a parent of 0 for a pid >= 2", m_par_zero, V_PARENT},
    {"two processes that are each other's parent", m_cycle, V_CYCLE},
    {"a process that is its own parent", m_self, V_CYCLE},
    {"a zombie with pending signals", m_pend_zombie, V_SIG_DEAD},
    {"a free slot with pending signals", m_pend_free, V_SIG_DEAD},
    {"SIGKILL blocked", m_block_kill, V_SIG_UNBLOCKABLE},
    {"SIGSTOP blocked in a zombie", m_block_stop_zombie, V_SIG_UNBLOCKABLE},
    {"signal bit 0 blocked", m_block_bit0, V_SIG_UNBLOCKABLE},
    {"init with a parent", m_init_parent, V_INIT},
};

// Reference exit, applied field by field to an expected-state copy.
static void r_exit(kstate *k, int pid, int status) {
    int parent = k->pt.p[pid].ppid;
    for (int q = 2; q < NPROC; q++) if (q != pid && k->pt.p[q].state != P_FREE && k->pt.p[q].ppid == pid) k->pt.p[q].ppid = 1;
    for (int fd = 0; fd < NFD; fd++) {
        int s = k->fdt[pid].slot[fd];
        if (s < 0) continue;
        k->fdt[pid].slot[fd] = -1;
        if (--k->files.files[s].refs == 0) { k->files.files[s].ino = 0; k->files.files[s].off = 0; }
    }
    k->sig[pid].pending = 0;
    k->pt.p[pid].state = P_ZOMBIE; k->pt.p[pid].status = status;
    // SIGCHLD reaches the parent's pending set unless it is ignored and unblocked
    int disp = k->sig[parent].disp[SG_CHLD], blocked = (k->sig[parent].blocked >> SG_CHLD) & 1;
    int ignored = disp == D_IGN || disp == D_DFL;
    if (!ignored || blocked) k->sig[parent].pending |= 1u << SG_CHLD;
}

static const char *state_diff(const kstate *a, const kstate *b) {
    static char w[160];
    for (int i = 1; i < NPROC; i++) {
        if (a->pt.p[i].state != b->pt.p[i].state || (b->pt.p[i].state != P_FREE && a->pt.p[i].ppid != b->pt.p[i].ppid)) { snprintf(w, sizeof w, "pid %d: state/parent %d/%d, expected %d/%d", i, a->pt.p[i].state, a->pt.p[i].ppid, b->pt.p[i].state, b->pt.p[i].ppid); return w; }
        if (b->pt.p[i].state == P_ZOMBIE && a->pt.p[i].status != b->pt.p[i].status) { snprintf(w, sizeof w, "pid %d status %d, expected %d", i, a->pt.p[i].status, b->pt.p[i].status); return w; }
        for (int fd = 0; fd < NFD; fd++) if (a->fdt[i].slot[fd] != b->fdt[i].slot[fd]) { snprintf(w, sizeof w, "pid %d fd %d slot %d, expected %d", i, fd, a->fdt[i].slot[fd], b->fdt[i].slot[fd]); return w; }
        if (a->sig[i].pending != b->sig[i].pending) { snprintf(w, sizeof w, "pid %d pending %#x, expected %#x", i, a->sig[i].pending, b->sig[i].pending); return w; }
    }
    for (int j = 0; j < NOFD; j++) {
        if (a->files.files[j].refs != b->files.files[j].refs) { snprintf(w, sizeof w, "file %d refs %d, expected %d", j, a->files.files[j].refs, b->files.files[j].refs); return w; }
        if (b->files.files[j].refs > 0 && (a->files.files[j].ino != b->files.files[j].ino || a->files.files[j].off != b->files.files[j].off)) { snprintf(w, sizeof w, "file %d contents changed", j); return w; }
    }
    return NULL;
}

static void random_state(kstate *k, unsigned seed) {
    t_rng = seed * 99991u;
    memset(k, 0, sizeof *k);
    for (int i = 0; i < NPROC; i++) for (int fd = 0; fd < NFD; fd++) k->fdt[i].slot[fd] = -1;
    set_proc(k, 1, P_RUN, 0);
    for (int pid = 2; pid < 12; pid++) {
        if (t_next() % 5 == 0) continue;
        int par; do par = 1 + t_next() % (pid - 1); while (k->pt.p[par].state != P_RUN);
        set_proc(k, pid, t_next() % 3 == 0 ? P_ZOMBIE : P_RUN, par);
        // a zombie must not have running children: keep the tree valid by only making leaves zombies later
    }
    for (int pid = 2; pid < NPROC; pid++) if (k->pt.p[pid].state == P_ZOMBIE)
        for (int q = 2; q < NPROC; q++) if (k->pt.p[q].state != P_FREE && k->pt.p[q].ppid == pid) k->pt.p[q].ppid = 1;
    for (int pid = 1; pid < NPROC; pid++) {
        if (k->pt.p[pid].state != P_RUN) continue;
        for (int fd = 0; fd < NFD; fd++) if (t_next() % 2) use_file(k, pid, fd, t_next() % NOFD, 1 + t_next() % 90);
        k->sig[pid].disp[SG_CHLD] = t_next() % 3;
        if (t_next() % 3 == 0) k->sig[pid].blocked = 1u << SG_CHLD;
        k->cr[pid] = (cred){1000, 1000, 100, 100, {0}, 0};
    }
}

int main(void) {
    kstate k;
    base_state(&k);
    CHECK("the reference agrees that the base state is consistent", r_check(&k) == 0, "the test's own base state is inconsistent");
    CHECK("a consistent state reports no violations", ks_check(&k) == 0, "reported %#x", ks_check(&k));
    for (unsigned i = 1; i < sizeof muts / sizeof muts[0]; i++) {
        base_state(&k);
        muts[i].apply(&k);
        uint32_t got = ks_check(&k);
        CHECK(muts[i].name, got == muts[i].expect, "reported %#x, expected exactly %#x", got, muts[i].expect);
    }

    // Defects can combine; every subset of up to three is compared with the reference.
    int bad = 0; char first[200] = ""; unsigned nm = sizeof muts / sizeof muts[0];
    for (unsigned a = 0; a < nm; a++) for (unsigned b = a; b < nm; b++) for (unsigned c = b; c < nm; c++) {
        base_state(&k); muts[a].apply(&k); muts[b].apply(&k); muts[c].apply(&k);
        uint32_t got = ks_check(&k), exp = r_check(&k);
        if (got != exp) { if (!bad) snprintf(first, sizeof first, "defects '%s' + '%s' + '%s': reported %#x, expected %#x", muts[a].name, muts[b].name, muts[c].name, got, exp); bad++; }
    }
    CHECK("combinations of up to three defects are reported exactly", bad == 0, "%d wrong, first: %s", bad, first);

    // ks_open
    base_state(&k);
    k.cr[3] = (cred){2000, 2000, 300, 300, {0}, 0};
    inode priv = {70, 1000, 100, 0640, 0}, pub = {71, 1000, 100, 0644, 0};
    kstate before = k;
    CHECK("a refused open returns -1 and changes nothing", ks_open(&k, 3, &priv, P_R) == -1 && memcmp(&before, &k, sizeof k) == 0, "state changed or wrong result");
    int fd = ks_open(&k, 3, &pub, P_R);
    CHECK("an allowed open takes the lowest free descriptor of that process", fd == 1 && k.files.files[k.fdt[3].slot[1]].ino == 71, "fd %d", fd);
    CHECK("the state stays consistent after the open", ks_check(&k) == 0 && r_check(&k) == 0, "reported %#x", ks_check(&k));
    CHECK("a write is refused where only read is allowed", ks_open(&k, 3, &pub, P_R | P_W) == -1, "allowed");
    CHECK("a zombie or missing process cannot open", ks_open(&k, 5, &pub, P_R) == -1 && ks_open(&k, 9, &pub, P_R) == -1 && ks_open(&k, 0, &pub, P_R) == -1 && ks_open(&k, 99, &pub, P_R) == -1, "allowed");
    base_state(&k);
    int last = 0; for (int i = 0; i < NFD; i++) last = ks_open(&k, 4, &pub, P_R);
    CHECK("a full table makes ks_open fail", last == -1, "the last open returned %d", last);

    // ks_exit on the hand-built state
    base_state(&k);
    kstate want = k;
    r_exit(&want, 2, 5);
    CHECK("exiting pid 2 closes its descriptors, orphans its children and notifies init", ks_exit(&k, 2, 5) == 0 && state_diff(&k, &want) == NULL, "%s", state_diff(&k, &want) ? state_diff(&k, &want) : "wrong result");
    CHECK("pid 2's exit leaves a consistent state", ks_check(&k) == 0, "reported %#x", ks_check(&k));
    CHECK("the children moved to init", k.pt.p[3].ppid == 1 && k.pt.p[4].ppid == 1, "parents %d and %d", k.pt.p[3].ppid, k.pt.p[4].ppid);
    base_state(&k);
    before = k;
    CHECK("init cannot exit, and a refused exit changes nothing", ks_exit(&k, 1, 0) == -1 && ks_exit(&k, 5, 0) == -1 && ks_exit(&k, 9, 0) == -1 && memcmp(&before, &k, sizeof k) == 0, "state changed");
    base_state(&k);
    k.sig[2].disp[SG_CHLD] = D_IGN; k.sig[2].pending = 0;
    ks_exit(&k, 3, 0);
    CHECK("an ignoring parent gets no pending SIGCHLD", k.sig[2].pending == 0, "pending %#x", k.sig[2].pending);
    base_state(&k);
    k.sig[2].disp[SG_CHLD] = D_DFL; k.sig[2].blocked |= 1u << SG_CHLD; k.sig[2].pending = 0;
    ks_exit(&k, 3, 0);
    CHECK("a blocking parent keeps SIGCHLD pending", k.sig[2].pending == 1u << SG_CHLD, "pending %#x", k.sig[2].pending);
    CHECK("pid 3's own pending signals were discarded", k.sig[3].pending == 0, "pending %#x", k.sig[3].pending);

    // Random whole-kernel runs: exits in random order keep the state consistent and match the reference.
    const char *why = NULL; static char msg[220];
    for (unsigned seed = 1; seed <= 40 && !why; seed++) {
        random_state(&k, seed);
        if (r_check(&k) != 0) { snprintf(msg, sizeof msg, "seed %u: the test's random state is inconsistent (%#x)", seed, r_check(&k)); why = msg; break; }
        kstate ex = k;
        for (int step = 0; step < 14 && !why; step++) {
            int pid = 1 + t_next() % 15, status = t_next() % 200;
            int ok = pid > 1 && ex.pt.p[pid].state == P_RUN;
            if (ok) r_exit(&ex, pid, status);
            int got = ks_exit(&k, pid, status);
            const char *d;
            if ((got == 0) != ok) { snprintf(msg, sizeof msg, "seed %u step %d: ks_exit(%d) returned %d, expected %s", seed, step, pid, got, ok ? "0" : "-1"); why = msg; }
            else if ((d = state_diff(&k, &ex)) != NULL) { snprintf(msg, sizeof msg, "seed %u step %d exit %d: %s", seed, step, pid, d); why = msg; }
            else if (ks_check(&k) != 0) { snprintf(msg, sizeof msg, "seed %u step %d exit %d: ks_check reports %#x on a consistent state", seed, step, pid, ks_check(&k)); why = msg; }
        }
    }
    CHECK("560 random exits keep the state consistent and match the reference", why == NULL, "%s", why ? why : "");
    return 0;
}
