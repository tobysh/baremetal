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

// Set a disposition. -1 for an invalid signal, SIGKILL/SIGSTOP, or an invalid disposition.
// If the new disposition ignores the signal, a pending instance is discarded.
int sig_set_disp(sproc *p, int sig, int disp) {
    return -1;
}

// Change the blocked mask; SIGKILL, SIGSTOP and bit 0 can never be blocked.
// Returns the previous mask. An invalid `how` changes nothing.
uint32_t sig_mask(sproc *p, int how, uint32_t set) {
    return 0;
}

// Generate signal `sig` for p. Returns -1 for an invalid number, otherwise 0.
int sig_send(sproc *p, int sig) {
    return -1;
}

// Deliver one signal: the lowest-numbered one that is pending and not blocked.
// Returns its number, or 0 if nothing was deliverable.
int sig_deliver(sproc *p) {
    return 0;
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
