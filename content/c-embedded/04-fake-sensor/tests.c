typedef struct { bus_t bus; uint32_t clock, done_at, raw, raw_lat; int started, fault_sched, fault, done, consumed; int starts, sreads, dreads, fwrites; uint32_t lastw; } S;
static void s_update(S *s) {
    if (s->started && s->clock >= s->done_at) { if (s->fault_sched) s->fault = 1; else if (!s->consumed) s->done = 1; }
}
static uint32_t s_read(bus_t *b, uint32_t off) {
    S *s = (S *)b;
    if (off == STATUS) { s_update(s); uint32_t v = (s->done ? 1u : 0u) | (s->fault ? 2u : 0u); s->sreads++; s->clock++; return v; }
    if (off == DATA) { s->dreads++; if (s->done) { s->done = 0; s->consumed = 1; s->started = 0; return 0xA5A5A000u | (s->raw & 0xFFFu); } return 0xDEAD0000u; }
    return 0;
}
static void s_write(bus_t *b, uint32_t off, uint32_t v) {
    S *s = (S *)b;
    if (off == CTRL && (v & 1u) && !s->fault) { s->starts++; s->started = 1; s->consumed = 0; s->done = 0; s->done_at = s->clock + s->raw_lat; }
    if (off == STATUS) { s->fwrites++; s->lastw = v; if (v & 2u) { s->fault = 0; s->started = 0; s->fault_sched = 0; } }
}
static uint32_t s_now(bus_t *b) { return ((S *)b)->clock; }

static void s_init(S *s, uint32_t clock, uint32_t raw, uint32_t lat, int fault) {
    memset(s, 0, sizeof *s);
    s->bus.read = s_read; s->bus.write = s_write; s->bus.now = s_now;
    s->clock = clock; s->raw = raw; s->raw_lat = lat; s->fault_sched = fault;
}
// Independent reference: arithmetic on the real value, no bit tricks.
static int32_t ref_mC(uint32_t raw) {
    int v = (int)(raw & 0xFFF);
    if (v >= 2048) v -= 4096;
    int neg = v < 0; if (neg) v = -v;
    int32_t mc = (v * 125 + 1) / 2;
    return neg ? -mc : mc;
}

int main(void) {
    static const uint32_t raws[] = { 0x000, 0x001, 0xFFF, 0x7FF, 0x800, 0x801, 0x190, 0xE70, 0x123, 0x003, 0xFFD, 0x400, 0xC00 };
    int val_ok = 1, ret_ok = 1, dr_ok = 1, st_ok = 1; char why[160] = "";
    for (unsigned i = 0; i < sizeof raws / sizeof raws[0]; i++) {
        S s; s_init(&s, i == 3 ? 0xFFFFFFFEu : 10 * i, raws[i], i % 4, 0);
        int32_t out = 123456789;
        int r = sensor_read_mC(&s.bus, &out, 8);
        if (r != 0) { ret_ok = 0; snprintf(why, sizeof why, "raw 0x%X returned %d", raws[i], r); }
        else if (out != ref_mC(raws[i]) && val_ok) { val_ok = 0; snprintf(why, sizeof why, "raw 0x%X gave %d mC, expected %d", raws[i], out, ref_mC(raws[i])); }
        if (s.dreads != 1) dr_ok = 0;
        if (s.starts != 1) st_ok = 0;
    }
    CHECK("readings decode to millidegrees, signs and rounding included", val_ok && ret_ok, "%s", why);
    CHECK("DATA is read exactly once on success", dr_ok, "wrong number of DATA reads");
    CHECK("START is written once", st_ok, "wrong number of START writes");

    S s; int32_t out;
    s_init(&s, 0, 0x190, 5, 0); out = -77;
    int r = sensor_read_mC(&s.bus, &out, 5);
    CHECK("latency equal to the timeout still succeeds", r == 0 && out == 25000, "returned %d, out %d", r, out);
    s_init(&s, 0xFFFFFFFAu, 0x190, 20, 0);
    r = sensor_read_mC(&s.bus, &out, 20);
    CHECK("a wrapping tick counter does not confuse the deadline", r == 0 && out == 25000, "returned %d", r);

    s_init(&s, 0, 0x190, 6, 0); out = -77;
    r = sensor_read_mC(&s.bus, &out, 5);
    CHECK("a slow sensor times out with -1", r == -1, "returned %d", r);
    CHECK("a timeout leaves *out alone and does not read DATA", out == -77 && s.dreads == 0, "out %d, %d DATA reads", out, s.dreads);
    CHECK("a dead sensor costs a bounded number of polls", (s_init(&s, 0, 0, 100000, 0), sensor_read_mC(&s.bus, &out, 50) == -1 && s.sreads <= 52), "%d STATUS reads", s.sreads);

    s_init(&s, 0, 0x190, 3, 1); out = -77;
    r = sensor_read_mC(&s.bus, &out, 20);
    CHECK("a fault returns -2", r == -2, "returned %d", r);
    CHECK("a fault is cleared by writing exactly the FAULT bit", s.fwrites == 1 && s.lastw == 2u && !s.fault, "%d writes, last 0x%X, fault still %d", s.fwrites, s.lastw, s.fault);
    CHECK("a fault leaves *out alone and does not read DATA", out == -77 && s.dreads == 0, "out %d, %d DATA reads", out, s.dreads);
    s.raw_lat = 2; s.raw = 0xE70;
    r = sensor_read_mC(&s.bus, &out, 20);
    CHECK("the driver works again after a fault", r == 0 && out == -25000, "returned %d, out %d", r, out);
    return 0;
}
