#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NSIG 32            // signal numbers 1..31
#define SG_HUP   1
#define SG_INT   2
#define SG_KILL  9
#define SG_USR1  10
#define SG_USR2  12
#define SG_TERM  15
#define SG_CHLD  17
#define SG_CONT  18
#define SG_STOP  19
#define SG_TSTP  20

enum { D_DFL, D_IGN, D_CATCH };            // disposition
enum { A_TERM, A_IGN, A_STOP };            // default action
enum { M_BLOCK, M_UNBLOCK, M_SETMASK };    // how to change the mask

typedef struct {
    int disp[NSIG];
    uint32_t pending, blocked;             // bit n is signal n
    int dead, term_sig, stopped;
    int handled[32]; int nhandled;         // log of signals whose handler "ran"
} sproc;

#define UNBLOCKABLE ((1u << SG_KILL) | (1u << SG_STOP))

static uint32_t bit(int sig) { return 1u << sig; }

// Default actions: HUP INT KILL USR1 USR2 TERM etc. terminate; CHLD and CONT are ignored;
// STOP and TSTP stop. Any signal number not listed terminates.
static int default_action(int sig) {
    if (sig == SG_CHLD || sig == SG_CONT) return A_IGN;
    if (sig == SG_STOP || sig == SG_TSTP) return A_STOP;
    return A_TERM;
}

static int is_ignored(const sproc *p, int sig) {
    return p->disp[sig] == D_IGN || (p->disp[sig] == D_DFL && default_action(sig) == A_IGN);
}

void sig_init(sproc *p) { memset(p, 0, sizeof *p); }

int sig_set_disp(sproc *p, int sig, int disp) {
    if (sig < 1 || sig >= NSIG || sig == SG_KILL || sig == SG_STOP) return -1;
    if (disp != D_DFL && disp != D_IGN && disp != D_CATCH) return -1;
    p->disp[sig] = disp;
    if (is_ignored(p, sig)) p->pending &= ~bit(sig);
    return 0;
}

uint32_t sig_mask(sproc *p, int how, uint32_t set) {
    uint32_t old = p->blocked;
    set &= ~(UNBLOCKABLE | 1u);
    if (how == M_BLOCK) p->blocked |= set;
    else if (how == M_UNBLOCK) p->blocked &= ~set;
    else if (how == M_SETMASK) p->blocked = set;
    return old;
}

int sig_send(sproc *p, int sig) {
    if (sig < 1 || sig >= NSIG) return -1;
    if (p->dead) return 0;
    if (sig == SG_CONT) { p->stopped = 0; p->pending &= ~(bit(SG_STOP) | bit(SG_TSTP)); }
    if (sig == SG_STOP || sig == SG_TSTP) p->pending &= ~bit(SG_CONT);
    if (is_ignored(p, sig) && !(p->blocked & bit(sig))) return 0;   // discarded at once
    p->pending |= bit(sig);                                          // standard signals coalesce
    return 0;
}

int sig_deliver(sproc *p) {
    if (p->dead) return 0;
    uint32_t ready = p->pending & ~p->blocked;
    if (p->stopped) ready &= bit(SG_KILL);          // a stopped process can only be killed
    if (!ready) return 0;
    int sig = 1;
    while (!(ready & bit(sig))) sig++;
    p->pending &= ~bit(sig);
    if (sig == SG_KILL) { p->dead = 1; p->term_sig = sig; p->pending = 0; return sig; }
    if (p->disp[sig] == D_CATCH) {
        if (p->nhandled < 32) p->handled[p->nhandled++] = sig;
    } else if (p->disp[sig] == D_DFL) {
        int a = default_action(sig);
        if (a == A_TERM) { p->dead = 1; p->term_sig = sig; p->pending = 0; }
        else if (a == A_STOP) p->stopped = 1;
    }
    return sig;
}

int main(void) {
    sproc p; sig_init(&p);
    sig_set_disp(&p, SG_USR1, D_CATCH);
    sig_mask(&p, M_BLOCK, bit(SG_USR1));
    sig_send(&p, SG_USR1); sig_send(&p, SG_USR1);        // two sends while blocked...
    printf("pending mask %#x, deliverable: %d\n", p.pending, sig_deliver(&p));
    sig_mask(&p, M_UNBLOCK, bit(SG_USR1));
    int s = sig_deliver(&p);
    printf("after unblocking: delivered %d, handler ran %d time(s)\n", s, p.nhandled);
    sig_send(&p, SG_TERM);
    sig_deliver(&p);
    printf("dead=%d by signal %d\n", p.dead, p.term_sig);
    return 0;
}
