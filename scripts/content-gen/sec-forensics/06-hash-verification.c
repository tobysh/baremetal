//@DATA@
int main(void) {
    for (size_t i = 0; i < sizeof V / sizeof V[0]; i++) {
        char name[64];
        snprintf(name, sizeof name, "fnv1a64 of input %zu (%zu bytes)", i + 1, V[i].n);
        uint64_t got = fnv1a64(V[i].p, V[i].n);
        CHECK(name, got == V[i].want, "expected %016llx, got %016llx", (unsigned long long)V[i].want, (unsigned long long)got);
    }
    for (size_t i = 0; i < sizeof A / sizeof A[0]; i++) {
        char name[64];
        int status[8];
        memset(status, 0x55, sizeof status);
        snprintf(name, sizeof name, "evidence set %zu (%d files)", i + 1, A[i].n);
        int bad = audit(A[i].manifest, A[i].items, A[i].n, status);
        int expect_bad = 0, ok = 1;
        for (int k = 0; k < A[i].n; k++) {
            if (A[i].want[k]) expect_bad++;
            if (status[k] != A[i].want[k]) ok = 0;
        }
        char msg[200] = "";
        int pos = 0;
        for (int k = 0; k < A[i].n; k++) pos += snprintf(msg + pos, sizeof msg - pos, "%s:%d(want %d) ", A[i].items[k].name, status[k], A[i].want[k]);
        CHECK(name, ok && bad == expect_bad, "returned %d (want %d); statuses %s", bad, expect_bad, msg);
    }
    return 0;
}
