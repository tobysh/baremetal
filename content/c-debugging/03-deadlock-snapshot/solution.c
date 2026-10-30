#include <stdio.h>
#include <string.h>

#define MAXHELD 4

typedef struct {
    int id;
    int held[MAXHELD];
    int nheld;
    int waiting;          // lock id, or -1 when not waiting
} thread_state;

// Index of the thread holding lock, or -1.
static int owner(const thread_state *t, int n, int lock) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < t[i].nheld; j++)
            if (t[i].held[j] == lock) return i;
    return -1;
}

static int next_of(const thread_state *t, int n, int i) {
    if (t[i].waiting < 0) return -1;
    return owner(t, n, t[i].waiting);
}

// Writes cycle thread ids into out, smallest id first. Returns length or 0.
int find_deadlock(const thread_state *t, int n, int *out) {
    int best = 0;
    int tmp[64];
    for (int s = 0; s < n; s++) {
        int cur = s;
        for (int k = 0; k < n && cur >= 0; k++) cur = next_of(t, n, cur);
        if (cur < 0) continue;              // chain ended: no cycle from here
        int len = 0, c = cur, minpos = 0;
        do {
            tmp[len] = t[c].id;
            if (tmp[len] < tmp[minpos]) minpos = len;
            len++;
            c = next_of(t, n, c);
        } while (c != cur);
        if (best == 0 || tmp[minpos] < out[0]) {
            for (int k = 0; k < len; k++) out[k] = tmp[(minpos + k) % len];
            best = len;
        }
    }
    return best;
}

// Locks the cycle waits for, ascending. Returns count or 0.
int suggest_order(const thread_state *t, int n, int *locks) {
    int ids[64];
    int len = find_deadlock(t, n, ids);
    for (int k = 0; k < len; k++) {
        for (int i = 0; i < n; i++)
            if (t[i].id == ids[k]) locks[k] = t[i].waiting;
    }
    for (int a = 1; a < len; a++) {
        int v = locks[a], b = a - 1;
        while (b >= 0 && locks[b] > v) { locks[b + 1] = locks[b]; b--; }
        locks[b + 1] = v;
    }
    return len;
}

// Run this to see the snapshot from the lesson analysed.
int main(void) {
    thread_state snap[4] = {
        {1, {'A'}, 1, 'B'},
        {2, {'B'}, 1, 'C'},
        {3, {'C'}, 1, 'A'},
        {4, {'D'}, 1, -1},
    };
    int out[4];
    int len = find_deadlock(snap, 4, out);
    printf("cycle length %d:", len);
    for (int i = 0; i < len; i++) printf(" %d", out[i]);
    printf("\n");
    return 0;
}
