typedef struct { bus_t bus; uint32_t clock, ready_at, step; int reads, writes, nows; } P;
static uint32_t p_read(bus_t *b, uint32_t off) {
    P *p = (P *)b; (void)off;
    uint32_t v = 0x10u | ((int32_t)(p->clock - p->ready_at) >= 0 ? 0x01u : 0u);   // bit 4 is a decoy
    p->reads++; p->clock += p->step;
    return v;
}
static void p_write(bus_t *b, uint32_t off, uint32_t v) { P *p = (P *)b; (void)off; (void)v; p->writes++; }
static uint32_t p_now(bus_t *b) { P *p = (P *)b; p->nows++; return p->clock; }

int main(void) {
    static const uint32_t starts[] = { 0u, 100u, 0xFFFFFFF0u, 0xFFFFFFFFu, 0x7FFFFFF8u };
    static const uint32_t steps[] = { 1, 3, 7 };
    static const uint32_t lat[] = { 0, 1, 2, 5, 9, 10, 11, 40 };
    static const uint32_t tmo[] = { 0, 1, 5, 10, 20 };
    int result_ok = 1, reads_ok = 1, nowrite_ok = 1; char why[160] = "";
    for (int a = 0; a < 5; a++) for (int s = 0; s < 3; s++) for (int l = 0; l < 8; l++) for (int t = 0; t < 5; t++) {
        P p = { { p_read, p_write, p_now }, starts[a], starts[a] + lat[l], steps[s], 0, 0, 0 };
        int r = wait_flag(&p.bus, 0, 0x01, tmo[t]);
        // Closed form. Reads happen at elapsed 0, step, 2*step, ... The device is ready at the first read
        // with k*step >= latency. A failed read k gives up when (k+1)*step > timeout.
        uint32_t k = (lat[l] + steps[s] - 1) / steps[s];
        int want = (k * steps[s] <= tmo[t]) ? 0 : -1;
        int want_reads = want == 0 ? (int)k + 1 : (int)(tmo[t] / steps[s]) + 1;
        if (r != want && result_ok) { result_ok = 0; snprintf(why, sizeof why, "start 0x%X step %u latency %u timeout %u: returned %d, expected %d", starts[a], steps[s], lat[l], tmo[t], r, want); }
        if (p.reads != want_reads && reads_ok) { reads_ok = 0; snprintf(why, sizeof why, "start 0x%X step %u latency %u timeout %u: %d reads, expected %d", starts[a], steps[s], lat[l], tmo[t], p.reads, want_reads); }
        if (p.writes != 0) nowrite_ok = 0;
    }
    CHECK("succeeds exactly when the device is ready in time", result_ok, "%s", why);
    CHECK("reads the register the expected number of times", reads_ok, "%s", why);
    CHECK("never writes to the device", nowrite_ok, "a register was written");

    P p = { { p_read, p_write, p_now }, 5, 1000000, 1, 0, 0, 0 };
    int r = wait_flag(&p.bus, 0, 1, 0);
    CHECK("timeout 0 still looks once, then gives up", r == -1 && p.reads == 1, "returned %d after %d reads", r, p.reads);

    P q = { { p_read, p_write, p_now }, 0xFFFFFFF0u, 0xFFFFFFFFu + 30u, 1, 0, 0, 0 };
    r = wait_flag(&q.bus, 0, 1, 100);
    CHECK("a counter that wraps mid-wait still counts elapsed time", r == 0 && q.reads <= 50, "returned %d after %d reads", r, q.reads);

    P d = { { p_read, p_write, p_now }, 0, 1u << 30, 1, 0, 0, 0 };
    r = wait_flag(&d.bus, 0, 1, 1000);
    CHECK("a dead device costs at most timeout + 1 reads", r == -1 && d.reads <= 1001, "%d reads", d.reads);

    P m = { { p_read, p_write, p_now }, 0, 0, 1, 0, 0, 0 };
    r = wait_flag(&m.bus, 0, 0x11, 5);   // both the decoy and the ready bit
    CHECK("every bit of the mask must be set", r == 0 && m.reads == 1, "returned %d after %d reads", r, m.reads);
    return 0;
}
