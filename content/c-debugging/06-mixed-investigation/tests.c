// Independent model of the expected behaviour.
typedef struct {
    cart c;
    unsigned char guard[16];   // sits right after the struct; must never change
} fenced;

static void fresh(fenced *f) {
    memset(f, 0, sizeof *f);
    memset(f->guard, 0xAA, sizeof f->guard);
}

static int guard_ok(const fenced *f) {
    for (size_t i = 0; i < sizeof f->guard; i++) if (f->guard[i] != 0xAA) return 0;
    return 1;
}

static int run(fenced *f, const int (*lines)[2], int n, int *rets) {
    fresh(f);
    for (int i = 0; i < n; i++) rets[i] = add_item(&f->c, lines[i][0], lines[i][1]);
    return n;
}

int main(void) {
    fenced f;
    int rets[32];

    // 1. The evidence sequence.
    const int ev[5][2] = {{2, 10}, {3, 5}, {1, 20}, {4, 2}, {7, 3}};
    run(&f, ev, 4, rets);
    CHECK("the total is quantity times price", f.c.total == 63, "total is %d after four lines, expected 63", f.c.total);
    CHECK("four lines are counted", f.c.used == 4 && rets[0] == 0 && rets[3] == 0, "used %d, returns %d %d", f.c.used, rets[0], rets[3]);
    CHECK("quantities are stored in order", f.c.qty[0] == 2 && f.c.qty[1] == 3 && f.c.qty[2] == 1 && f.c.qty[3] == 4, "qty = %d %d %d %d", f.c.qty[0], f.c.qty[1], f.c.qty[2], f.c.qty[3]);

    run(&f, ev, 5, rets);
    CHECK("the fifth line is refused", rets[4] == -1, "returned %d, expected -1", rets[4]);
    CHECK("a refused line leaves used at 4", f.c.used == 4, "used is %d, expected 4", f.c.used);
    CHECK("a refused line leaves the total alone", f.c.total == 63, "total is %d, expected 63", f.c.total);
    CHECK("a refused line writes nothing after the struct", guard_ok(&f), "a guard byte changed");

    // 2. Different shapes: zero values, a single line, a price of zero.
    const int one[1][2] = {{5, 7}};
    run(&f, one, 1, rets);
    CHECK("a single line", f.c.used == 1 && f.c.total == 35 && rets[0] == 0, "used %d total %d", f.c.used, f.c.total);

    const int zeros[4][2] = {{0, 9}, {6, 0}, {0, 0}, {1, 1}};
    run(&f, zeros, 4, rets);
    CHECK("zero quantity and zero price still count as lines", f.c.used == 4 && f.c.total == 1, "used %d total %d, expected 4 and 1", f.c.used, f.c.total);

    // 3. Pushing a full cart many times, with quantities that would alias other counters.
    const int many[12][2] = {{1, 1}, {1, 2}, {1, 3}, {1, 4}, {100, 5}, {0, 5}, {2, 9}, {3, 9}, {-1, 4}, {1000, 1}, {5, 5}, {6, 6}};
    int n = run(&f, many, 12, rets);
    int refused = 0;
    for (int i = 4; i < n; i++) if (rets[i] == -1) refused++;
    CHECK("every later line is refused", refused == 8, "%d of 8 later lines refused", refused);
    CHECK("repeated refusals keep used at 4", f.c.used == 4, "used is %d, expected 4", f.c.used);
    CHECK("repeated refusals keep the total at 10", f.c.total == 10, "total is %d, expected 10 (1+2+3+4)", f.c.total);
    CHECK("repeated refusals never touch the bytes after the struct", guard_ok(&f), "a guard byte changed");
    CHECK("stored quantities survive refusals", f.c.qty[0] == 1 && f.c.qty[3] == 1, "qty[0]=%d qty[3]=%d", f.c.qty[0], f.c.qty[3]);

    // 4. A cart must be reusable after a reset by the caller.
    memset(&f.c, 0, sizeof f.c);
    int r = add_item(&f.c, 3, 4);
    CHECK("a cleared cart accepts lines again", r == 0 && f.c.used == 1 && f.c.total == 12, "returned %d used %d total %d", r, f.c.used, f.c.total);

    // 5. The diagnosis: the root cause of the corrupted `used`, not the visible cost error.
    diagnosis_code d = diagnosis();
    CHECK("a diagnosis is given", d != DIAG_UNKNOWN, "diagnosis() still returns DIAG_UNKNOWN");
    CHECK("the cost formula does not explain used jumping to 8", d != DIAG_COST_IGNORES_QUANTITY, "that fault is real but it cannot change `used`; look at call 5 again");
    CHECK("the root cause of the corruption is named", d == DIAG_MISSING_CAPACITY_CHECK, "diagnosis code %d is not the cause of the corrupted counter", (int)d);
    return 0;
}
