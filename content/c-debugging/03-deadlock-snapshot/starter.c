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

// Writes cycle thread ids into out, smallest id first. Returns length or 0.
int find_deadlock(const thread_state *t, int n, int *out) {
    (void)t; (void)n; (void)out;
    return 0;
}

// Locks the cycle waits for, ascending. Returns count or 0.
int suggest_order(const thread_state *t, int n, int *locks) {
    (void)t; (void)n; (void)locks;
    return 0;
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
