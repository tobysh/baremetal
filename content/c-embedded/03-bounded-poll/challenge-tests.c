typedef struct { bus_t bus; uint32_t clock, ra, rb; } Q;
static uint32_t q_read(bus_t *b, uint32_t off) {
    Q *q = (Q *)b;
    uint32_t at = off == 0 ? q->ra : q->rb;
    uint32_t v = (int32_t)(q->clock - at) >= 0 ? 1u : 0u;
    q->clock++;
    return v;
}
static void q_write(bus_t *b, uint32_t off, uint32_t v) { (void)b; (void)off; (void)v; }
static uint32_t q_now(bus_t *b) { return ((Q *)b)->clock; }

// Independent reference: explicit time variable, absolute ready ticks relative to entry.
static int ref(uint32_t ta, uint32_t tb, uint32_t timeout) {
    uint32_t t = 0;
    for (;;) { int r = t >= ta; t++; if (r) break; if (t > timeout) return -1; }
    for (;;) { int r = t >= tb; t++; if (r) return 0; if (t > timeout) return -2; }
}

int main(void) {
    struct { uint32_t ta, tb, to; int want; } hand[] = {
        {3, 3, 6, 0}, {5, 5, 6, 0}, {5, 9, 7, -2}, {9, 0, 5, -1}, {0, 0, 0, 0}, {0, 30, 10, -2}, {2, 2, 100, 0},
    };
    int ok = 1; char why[160] = "";
    for (unsigned i = 0; i < sizeof hand / sizeof hand[0]; i++) {
        Q q = { { q_read, q_write, q_now }, 0xFFFFFFFCu, 0xFFFFFFFCu + hand[i].ta, 0xFFFFFFFCu + hand[i].tb };
        int r = wait_two(&q.bus, 0, 1, 4, 1, hand[i].to);
        if (r != hand[i].want && ok) { ok = 0; snprintf(why, sizeof why, "A at %u, B at %u, timeout %u: returned %d, expected %d", hand[i].ta, hand[i].tb, hand[i].to, r, hand[i].want); }
    }
    CHECK("hand-worked cases", ok, "%s", why);
    int ok2 = 1; char why2[160] = "";
    for (uint32_t ta = 0; ta < 14; ta++) for (uint32_t tb = 0; tb < 14; tb++) for (uint32_t to = 0; to < 16; to += 3) {
        Q q = { { q_read, q_write, q_now }, 77, 77 + ta, 77 + tb };
        int r = wait_two(&q.bus, 0, 1, 4, 1, to), w = ref(ta, tb, to);
        if (r != w && ok2) { ok2 = 0; snprintf(why2, sizeof why2, "A %u B %u timeout %u: returned %d, expected %d", ta, tb, to, r, w); }
    }
    CHECK("the budget is shared across both waits", ok2, "%s", why2);
    Q q = { { q_read, q_write, q_now }, 0, 4, 9 };
    CHECK("the second wait does not get a fresh budget", wait_two(&q.bus, 0, 1, 4, 1, 7) == -2, "A ready at 4, B at 9, budget 7 must fail with -2");
    return 0;
}
