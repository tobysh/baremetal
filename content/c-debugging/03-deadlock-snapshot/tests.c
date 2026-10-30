static thread_state mk(int id, int nheld, int a, int b, int waiting) {
    thread_state x;
    memset(&x, 0, sizeof x);
    x.id = id;
    x.nheld = nheld;
    x.held[0] = a;
    x.held[1] = b;
    x.waiting = waiting;
    return x;
}

static int same(const int *got, int len, const int *want, int wlen) {
    if (len != wlen) return 0;
    for (int i = 0; i < len; i++) if (got[i] != want[i]) return 0;
    return 1;
}

int main(void) {
    int out[16], lk[16];

    // 1. nobody waits
    {
        thread_state s[3] = { mk(1, 1, 10, 0, -1), mk(2, 1, 11, 0, -1), mk(3, 0, 0, 0, -1) };
        for (int i = 0; i < 16; i++) out[i] = -7;
        int r = find_deadlock(s, 3, out);
        CHECK("no waiting threads means no deadlock", r == 0, "returned %d, expected 0", r);
        CHECK("out is left alone without a deadlock", out[0] == -7, "out[0] changed to %d", out[0]);
        CHECK("no deadlock gives no lock order", suggest_order(s, 3, lk) == 0, "suggest_order returned non-zero");
    }

    // 2. two-thread cycle, ids out of array order
    {
        thread_state s[2] = { mk(9, 1, 2, 0, 1), mk(4, 1, 1, 0, 2) };
        int want[] = {4, 9};
        int r = find_deadlock(s, 2, out);
        CHECK("two-thread cycle length", r == 2, "returned %d, expected 2", r);
        CHECK("two-thread cycle starts at the smallest id", same(out, r, want, 2), "got %d %d, expected 4 9", out[0], out[1]);
        int m = suggest_order(s, 2, lk);
        CHECK("two-thread lock order is ascending", m == 2 && lk[0] == 1 && lk[1] == 2, "got %d locks: %d %d, expected 1 2", m, lk[0], lk[1]);
    }

    // 3. three-thread cycle, rotated start in the array
    {
        thread_state s[3] = { mk(30, 1, 300, 0, 100), mk(10, 1, 100, 0, 200), mk(20, 1, 200, 0, 300) };
        int want[] = {10, 20, 30};
        int r = find_deadlock(s, 3, out);
        CHECK("three-thread cycle length", r == 3, "returned %d, expected 3", r);
        CHECK("three-thread cycle follows the arrows from the smallest id", same(out, r, want, 3), "got %d %d %d, expected 10 20 30", out[0], out[1], out[2]);
        int m = suggest_order(s, 3, lk);
        CHECK("three-thread lock order is sorted", m == 3 && lk[0] == 100 && lk[1] == 200 && lk[2] == 300, "got %d locks: %d %d %d", m, lk[0], lk[1], lk[2]);
    }

    // 3b. direction matters: 1 -> 3 -> 2 -> 1
    {
        thread_state s[3] = { mk(1, 1, 1, 0, 3), mk(2, 1, 2, 0, 1), mk(3, 1, 3, 0, 2) };
        int want[] = {1, 3, 2};
        int r = find_deadlock(s, 3, out);
        CHECK("the cycle order follows who waits for whom, not id order", same(out, r, want, 3), "got %d %d %d, expected 1 3 2", out[0], out[1], out[2]);
    }

    // 4. waiting chain that is not a cycle
    {
        thread_state s[4] = { mk(1, 1, 1, 0, 2), mk(2, 1, 2, 0, 3), mk(3, 1, 3, 0, 4), mk(4, 1, 4, 0, -1) };
        for (int i = 0; i < 16; i++) out[i] = -7;
        int r = find_deadlock(s, 4, out);
        CHECK("a chain ending at a running thread is not a deadlock", r == 0, "returned %d, expected 0", r);
        CHECK("a chain leaves out alone", out[0] == -7, "out[0] changed to %d", out[0]);
        CHECK("a chain gives no lock order", suggest_order(s, 4, lk) == 0, "suggest_order returned non-zero");
    }

    // 5. cycle plus bystanders (one waits into the cycle, one is idle, one waits for a free lock)
    {
        thread_state s[6] = {
            mk(7, 1, 70, 0, -1),
            mk(5, 2, 50, 51, 60),
            mk(8, 1, 80, 0, 50),     // waits on the cycle, not part of it
            mk(6, 1, 60, 0, 51),
            mk(2, 0, 0, 0, 999),     // lock 999 held by nobody
            mk(3, 1, 30, 0, 70),     // waits for the idle thread 7
        };
        int want[] = {5, 6};
        int r = find_deadlock(s, 6, out);
        CHECK("bystanders are not reported", r == 2, "returned %d, expected 2", r);
        CHECK("the cycle is 5 then 6", same(out, r, want, 2), "got %d %d, expected 5 6", out[0], out[1]);
        int m = suggest_order(s, 6, lk);
        CHECK("lock order ignores bystander locks", m == 2 && lk[0] == 51 && lk[1] == 60, "got %d locks: %d %d, expected 51 60", m, lk[0], lk[1]);
    }

    // 6. waiting on a lock nobody holds
    {
        thread_state s[2] = { mk(1, 1, 1, 0, 42), mk(2, 1, 2, 0, -1) };
        int r = find_deadlock(s, 2, out);
        CHECK("waiting for a lock nobody holds is not a cycle", r == 0, "returned %d, expected 0", r);
    }

    // 7. a thread waits for its own lock
    {
        thread_state s[2] = { mk(3, 0, 0, 0, -1), mk(12, 2, 5, 6, 6) };
        int r = find_deadlock(s, 2, out);
        CHECK("waiting for a lock you hold is a cycle of one", r == 1 && out[0] == 12, "returned %d, out[0]=%d, expected 1 and 12", r, out[0]);
        int m = suggest_order(s, 2, lk);
        CHECK("self-deadlock names the lock", m == 1 && lk[0] == 6, "got %d locks, lk[0]=%d, expected 1 and 6", m, lk[0]);
    }

    // 8. locks held as a second entry, and two separate cycles
    {
        thread_state s[4] = {
            mk(20, 2, 1, 2, 3),      // holds 1,2 waits 3
            mk(21, 1, 3, 0, 1),      // holds 3 waits 1  -> cycle 20,21
            mk(10, 1, 7, 0, 8),
            mk(11, 1, 8, 0, 7),      // cycle 10,11 has the smaller id
        };
        int want[] = {10, 11};
        int r = find_deadlock(s, 4, out);
        CHECK("with two cycles the one with the smallest id wins", same(out, r, want, 2), "length %d, first id %d, expected 2 and 10", r, out[0]);
        int m = suggest_order(s, 4, lk);
        CHECK("the lock order matches the reported cycle", m == 2 && lk[0] == 7 && lk[1] == 8, "got %d locks: %d %d, expected 7 8", m, lk[0], lk[1]);
    }

    // 9. a long ring of ten
    {
        thread_state s[10];
        for (int i = 0; i < 10; i++) s[i] = mk(100 + i, 1, 500 + i, 0, 500 + (i + 1) % 10);
        int r = find_deadlock(s, 10, out);
        int ok = r == 10;
        for (int i = 0; ok && i < 10; i++) ok = out[i] == 100 + i;
        CHECK("a ring of ten threads is reported in order", ok, "returned %d, out[0]=%d", r, out[0]);
    }
    return 0;
}
