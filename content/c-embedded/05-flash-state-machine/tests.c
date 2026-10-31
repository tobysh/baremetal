typedef struct {
    bus_t bus; uint32_t clock, busy_until, dur;
    int lock;                      // 0 locked, 1 half, 2 unlocked, 3 sticky
    int err, pending, fails_left;  // transient failures still to inject
    uint32_t paddr, pdata, flash[32];
    int attempts, dropped, key_writes, errclr, lastw_ok;
} F;
static void f_update(F *f) {
    if (f->pending && f->clock >= f->busy_until) {
        f->pending = 0;
        uint32_t w = f->paddr / 4;
        int bad = f->lock != 2 || (f->paddr & 3) || f->paddr >= 0x60 || f->flash[w] != 0xFFFFFFFFu;
        if (f->fails_left > 0) { f->fails_left--; bad = 1; }
        if (bad) f->err = 1; else f->flash[w] = f->pdata;
    }
}
static int f_busy(F *f) { return f->pending && f->clock < f->busy_until; }
static uint32_t f_read(bus_t *b, uint32_t off) {
    F *f = (F *)b;
    if (off != 0x0C) return 0;
    f_update(f);
    uint32_t v = (f_busy(f) ? 1u : 0u) | (f->err ? 2u : 0u) | (f->lock != 2 ? 4u : 0u);
    f->clock++;
    return v;
}
static void f_write(bus_t *b, uint32_t off, uint32_t v) {
    F *f = (F *)b;
    f_update(f);
    if (off == 0x10) {
        f->key_writes++;
        if (f->lock == 0) { if (v == 0x45670123u) f->lock = 1; }
        else if (f->lock == 1) f->lock = (v == 0xCDEF89ABu) ? 2 : 3;
        else f->lock = 3;
    } else if (off == 0x0C) { if (v & 2u) { f->err = 0; f->errclr++; f->lastw_ok = (v == 2u); } }
    else if (f_busy(f)) f->dropped++;
    else if (off == 0x04) f->paddr = v;
    else if (off == 0x08) f->pdata = v;
    else if (off == 0x00) {
        if (v == 1 && !f->err) { f->attempts++; f->pending = 1; f->busy_until = f->clock + f->dur; }
        else f->dropped++;
    }
}
static uint32_t f_now(bus_t *b) { return ((F *)b)->clock; }
static void f_init(F *f, uint32_t dur, int fails) {
    memset(f, 0, sizeof *f);
    f->bus.read = f_read; f->bus.write = f_write; f->bus.now = f_now;
    f->dur = dur; f->fails_left = fails; f->lastw_ok = 1;
    for (int i = 0; i < 32; i++) f->flash[i] = 0xFFFFFFFFu;
}

int main(void) {
    F f; int r;
    f_init(&f, 3, 0);
    r = flash_program(&f.bus, 0x10, 0x11223344u, 20);
    CHECK("a program on a locked controller succeeds", r == 0 && f.flash[4] == 0x11223344u, "returned %d, word is 0x%08X", r, f.flash[4]);
    CHECK("exactly one attempt when nothing fails", f.attempts == 1 && f.errclr == 0, "%d attempts, %d ERR clears", f.attempts, f.errclr);

    static const uint32_t addrs[] = { 0x20, 0x24, 0x5C };
    static const uint32_t datas[] = { 0xDEADBEEFu, 0x00000000u, 0x80000001u };
    f_init(&f, 2, 0);
    int ok = 1;
    for (int i = 0; i < 3; i++) if (flash_program(&f.bus, addrs[i], datas[i], 20) != 0) ok = 0;
    CHECK("several programs in a row land at the right addresses", ok && f.flash[8] == datas[0] && f.flash[9] == datas[1] && f.flash[23] == datas[2], "words: 0x%08X 0x%08X 0x%08X", f.flash[8], f.flash[9], f.flash[23]);
    CHECK("the keys are written once in total", f.key_writes == 2 && f.lock == 2, "%d key writes, lock state %d (3 = sticky)", f.key_writes, f.lock);

    f_init(&f, 4, 0); f.lock = 2;
    CHECK("flash_unlock on an unlocked controller writes no keys", flash_unlock(&f.bus) == 0 && f.key_writes == 0, "%d key writes", f.key_writes);
    f_init(&f, 3, 0);
    CHECK("flash_unlock unlocks a locked controller", flash_unlock(&f.bus) == 0 && f.lock == 2, "lock state %d", f.lock);
    f_init(&f, 3, 0); f.lock = 3;
    CHECK("a sticky lock is reported with -4", flash_unlock(&f.bus) == -4 && f.attempts == 0, "stuck controller not reported");

    for (int fails = 1; fails <= 2; fails++) {
        f_init(&f, 3, fails);
        r = flash_program(&f.bus, 0x30, 0xA0A0A0A0u, 20);
        char nm[80]; snprintf(nm, sizeof nm, "%d transient failure(s) are retried and cleared", fails);
        CHECK(nm, r == 0 && f.attempts == fails + 1 && f.errclr == fails && f.flash[12] == 0xA0A0A0A0u,
              "returned %d, %d attempts, %d ERR clears, word 0x%08X", r, f.attempts, f.errclr, f.flash[12]);
        CHECK(fails == 1 ? "ERR is cleared by writing exactly the ERR bit" : "after retries the controller is not left in ERR", f.lastw_ok && !f.err, "last STATUS write was not exactly ERR, or ERR left set");
    }

    f_init(&f, 3, 3);
    r = flash_program(&f.bus, 0x30, 0x1u, 20);
    CHECK("three transient failures give up with -3", r == -3 && f.attempts == 3, "returned %d after %d attempts", r, f.attempts);
    f_init(&f, 3, 0);
    r = flash_program(&f.bus, 0x70, 0x1u, 20);
    CHECK("a protected address fails after exactly 3 attempts", r == -3 && f.attempts == 3, "returned %d after %d attempts", r, f.attempts);
    f_init(&f, 3, 0); f.flash[4] = 0x12345678u;
    r = flash_program(&f.bus, 0x10, 0x1u, 20);
    CHECK("a non-blank word is not overwritten", r == -3 && f.flash[4] == 0x12345678u, "returned %d, word 0x%08X", r, f.flash[4]);

    f_init(&f, 50, 0);
    r = flash_program(&f.bus, 0x10, 0x1u, 10);
    CHECK("a controller stuck busy gives -1 without retrying", r == -1 && f.attempts == 1, "returned %d after %d attempts", r, f.attempts);
    f_init(&f, 3, 0); f.pending = 1; f.busy_until = 6; f.paddr = 0x40; f.pdata = 0; f.lock = 2;
    r = flash_program(&f.bus, 0x14, 0xBEEF0001u, 30);
    CHECK("a controller that is already busy is waited for", r == 0 && f.flash[5] == 0xBEEF0001u && f.dropped == 0, "returned %d, word 0x%08X, %d dropped writes", r, f.flash[5], f.dropped);
    return 0;
}
