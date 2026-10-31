// ---- reference: one flag per signal instead of bit masks ----
typedef struct { int pend[NSIG], blk[NSIG], disp[NSIG], dead, term, stopped, log[64], nlog; } rproc;

static int r_ign(const rproc *r, int s) {
    return r->disp[s] == D_IGN || (r->disp[s] == D_DFL && (s == SG_CHLD || s == SG_CONT));
}
static int r_set_disp(rproc *r, int s, int d) {
    if (s < 1 || s > 31 || s == 9 || s == 19 || d < 0 || d > 2) return -1;
    r->disp[s] = d;
    if (r_ign(r, s)) r->pend[s] = 0;
    return 0;
}
static uint32_t r_mask(rproc *r, int how, uint32_t set) {
    uint32_t old = 0;
    for (int s = 1; s < NSIG; s++) if (r->blk[s]) old |= 1u << s;
    for (int s = 1; s < NSIG; s++) {
        if (s == 9 || s == 19) continue;
        int in = (set >> s) & 1;
        if (how == M_BLOCK && in) r->blk[s] = 1;
        if (how == M_UNBLOCK && in) r->blk[s] = 0;
        if (how == M_SETMASK) r->blk[s] = in;
    }
    return old;
}
static int r_send(rproc *r, int s) {
    if (s < 1 || s > 31) return -1;
    if (r->dead) return 0;
    if (s == 18) { r->stopped = 0; r->pend[19] = 0; r->pend[20] = 0; }
    if (s == 19 || s == 20) r->pend[18] = 0;
    if (r_ign(r, s) && !r->blk[s]) return 0;
    r->pend[s] = 1;
    return 0;
}
static int r_deliver(rproc *r) {
    if (r->dead) return 0;
    for (int s = 1; s < NSIG; s++) {
        if (!r->pend[s] || r->blk[s]) continue;
        if (r->stopped && s != 9) continue;
        r->pend[s] = 0;
        if (s == 9) { r->dead = 1; r->term = 9; for (int k = 0; k < NSIG; k++) r->pend[k] = 0; return 9; }
        if (r->disp[s] == D_CATCH) { if (r->nlog < 32) r->log[r->nlog++] = s; }
        else if (r->disp[s] == D_DFL) {
            if (s == 19 || s == 20) r->stopped = 1;
            else if (s != 17 && s != 18) { r->dead = 1; r->term = s; for (int k = 0; k < NSIG; k++) r->pend[k] = 0; }
        }
        return s;
    }
    return 0;
}

static const char *same(const sproc *p, const rproc *r) {
    static char w[160];
    uint32_t pend = 0, blk = 0;
    for (int s = 1; s < NSIG; s++) { if (r->pend[s]) pend |= 1u << s; if (r->blk[s]) blk |= 1u << s; }
    if (p->pending != pend) { snprintf(w, sizeof w, "pending is %#x, expected %#x", p->pending, pend); return w; }
    if (p->blocked != blk) { snprintf(w, sizeof w, "blocked is %#x, expected %#x", p->blocked, blk); return w; }
    if (p->dead != r->dead || (r->dead && p->term_sig != r->term)) { snprintf(w, sizeof w, "dead/term_sig is %d/%d, expected %d/%d", p->dead, p->term_sig, r->dead, r->term); return w; }
    if (p->stopped != r->stopped) { snprintf(w, sizeof w, "stopped is %d, expected %d", p->stopped, r->stopped); return w; }
    if (p->nhandled != r->nlog) { snprintf(w, sizeof w, "handler ran %d times, expected %d", p->nhandled, r->nlog); return w; }
    for (int i = 0; i < r->nlog; i++) if (p->handled[i] != r->log[i]) { snprintf(w, sizeof w, "handler log entry %d is %d, expected %d", i, p->handled[i], r->log[i]); return w; }
    return NULL;
}

static unsigned t_rng = 1;
static unsigned t_next(void) { t_rng = t_rng * 1103515245u + 12345u; return (t_rng >> 16) & 0x7fff; }

int main(void) {
    sproc p; sig_init(&p);
    CHECK("a blocked signal stays pending and coalesces",
          (sig_mask(&p, M_BLOCK, bit(SG_USR1)), sig_send(&p, SG_USR1), sig_send(&p, SG_USR1), p.pending == bit(SG_USR1) && sig_deliver(&p) == 0),
          "pending %#x", p.pending);
    sig_set_disp(&p, SG_USR1, D_CATCH);
    sig_mask(&p, M_UNBLOCK, bit(SG_USR1));
    int d = sig_deliver(&p);
    CHECK("unblocking delivers once, not twice", d == SG_USR1 && p.nhandled == 1 && sig_deliver(&p) == 0, "delivered %d, handler ran %d times", d, p.nhandled);

    sig_init(&p);
    sig_send(&p, SG_TERM); sig_send(&p, SG_INT); sig_send(&p, SG_HUP);
    CHECK("lowest-numbered pending signal goes first", sig_deliver(&p) == SG_HUP && p.dead && p.term_sig == SG_HUP, "dead %d term_sig %d", p.dead, p.term_sig);
    CHECK("a dead process receives nothing", (sig_send(&p, SG_USR1), p.pending == 0) && sig_deliver(&p) == 0, "pending %#x", p.pending);

    sig_init(&p);
    uint32_t old = sig_mask(&p, M_SETMASK, 0xFFFFFFFFu);
    CHECK("KILL, STOP and bit 0 can never be blocked", p.blocked == (0xFFFFFFFFu & ~(bit(SG_KILL) | bit(SG_STOP) | 1u)) && old == 0, "blocked %#x", p.blocked);
    sig_send(&p, SG_KILL);
    CHECK("SIGKILL kills despite a full mask", sig_deliver(&p) == SG_KILL && p.dead, "dead %d", p.dead);

    sig_init(&p);
    CHECK("KILL and STOP cannot be caught or ignored", sig_set_disp(&p, SG_KILL, D_IGN) == -1 && sig_set_disp(&p, SG_STOP, D_CATCH) == -1 && sig_set_disp(&p, 0, D_IGN) == -1 && sig_set_disp(&p, 32, D_IGN) == -1 && sig_set_disp(&p, SG_USR1, 7) == -1, "an invalid change was accepted");
    sig_mask(&p, M_BLOCK, bit(SG_USR2));
    sig_send(&p, SG_USR2);
    sig_set_disp(&p, SG_USR2, D_IGN);
    CHECK("ignoring a signal discards the pending instance", p.pending == 0, "pending %#x", p.pending);
    sig_send(&p, SG_CHLD);
    CHECK("a default-ignored signal is dropped when unblocked", p.pending == 0, "pending %#x", p.pending);
    sig_mask(&p, M_BLOCK, bit(SG_CHLD));
    sig_send(&p, SG_CHLD);
    CHECK("but is kept while blocked", p.pending == bit(SG_CHLD), "pending %#x", p.pending);
    CHECK("and then discarded silently on delivery", sig_deliver(&p) == 0 && p.pending == bit(SG_CHLD), "a blocked signal was delivered");
    sig_mask(&p, M_UNBLOCK, bit(SG_CHLD));
    CHECK("unblocking an ignored pending signal delivers it as a no-op", sig_deliver(&p) == SG_CHLD && !p.dead && p.pending == 0, "dead %d", p.dead);

    sig_init(&p);
    sig_send(&p, SG_TSTP);
    CHECK("TSTP stops the process", sig_deliver(&p) == SG_TSTP && p.stopped, "stopped %d", p.stopped);
    sig_send(&p, SG_USR1);
    CHECK("a stopped process takes no ordinary signals", sig_deliver(&p) == 0 && p.pending == bit(SG_USR1), "pending %#x", p.pending);
    sig_send(&p, SG_CONT);
    CHECK("CONT resumes immediately, whatever the disposition", !p.stopped && p.pending == bit(SG_USR1), "stopped %d pending %#x", p.stopped, p.pending);
    sig_send(&p, SG_STOP); sig_send(&p, SG_CONT);
    CHECK("CONT discards a pending stop signal", p.pending == bit(SG_USR1), "pending %#x", p.pending);

    // Random schedules against the flag-per-signal reference.
    const char *bad = NULL; static char msg[220];
    for (unsigned seed = 1; seed <= 10 && !bad; seed++) {
        sproc q; rproc r; sig_init(&q); memset(&r, 0, sizeof r); t_rng = seed * 2654435761u;
        for (int step = 0; step < 300 && !bad; step++) {
            int op = t_next() % 6, s = (int)(t_next() % 33) - 0;
            uint32_t set = (t_next() << 16) ^ t_next(); if (t_next() % 3 == 0) set &= 0x000FFFFFu >> (t_next() % 8);
            int how = t_next() % 4, disp = t_next() % 4, got, want;
            if (op == 0) { got = sig_set_disp(&q, s, disp); want = r_set_disp(&r, s, disp); }
            else if (op == 1) { got = (int)sig_mask(&q, how, set); want = (int)r_mask(&r, how, set); }
            else if (op == 2 || op == 3) { got = sig_send(&q, s); want = r_send(&r, s); }
            else { got = sig_deliver(&q); want = r_deliver(&r); }
            const char *w;
            if (got != want) { snprintf(msg, sizeof msg, "seed %u step %d op %d (sig %d): returned %d, expected %d", seed, step, op, s, got, want); bad = msg; }
            else if ((w = same(&q, &r)) != NULL) { snprintf(msg, sizeof msg, "seed %u step %d op %d (sig %d): %s", seed, step, op, s, w); bad = msg; }
        }
    }
    CHECK("3000 random steps match the reference", bad == NULL, "%s", bad ? bad : "");
    return 0;
}
