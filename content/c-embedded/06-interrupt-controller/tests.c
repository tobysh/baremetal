typedef struct { bus_t bus; uint32_t enable, pending; int acks; uint32_t ack_log[64]; int reads_p, reads_e, enable_reads, enable_writes; } I;
static uint32_t i_read(bus_t *b, uint32_t off) {
    I *i = (I *)b;
    if (off == 0x00) { i->reads_e++; i->enable_reads++; return i->enable; }
    if (off == 0x04) { i->reads_p++; return i->pending; }
    return 0;
}
static void i_write(bus_t *b, uint32_t off, uint32_t v) {
    I *i = (I *)b;
    if (off == 0x00) { i->enable = v; i->enable_writes++; }
    else if (off == 0x08) { if (i->acks < 64) i->ack_log[i->acks] = v; i->acks++; i->pending &= ~v; }
}
static uint32_t i_now(bus_t *b) { (void)b; return 0; }
static void i_init(I *i) { memset(i, 0, sizeof *i); i->bus.read = i_read; i->bus.write = i_write; i->bus.now = i_now; }

typedef struct { I *intc; int line; int calls; int refire; int order_pos; int *order; int marker; } H;
static int g_order[64], g_n;
static void h_run(void *ctx) {
    H *h = (H *)ctx;
    h->calls++;
    if (g_n < 64) g_order[g_n++] = h->line;
    if (h->refire && h->calls <= h->refire) h->intc->pending |= 1u << h->line;   // new event during the handler
}
static void h_stuck(void *ctx) { H *h = (H *)ctx; h->calls++; h->intc->pending |= 1u << h->line; }
static void h_noop(void *ctx) { (void)ctx; }

int main(void) {
    I i; irq_table_t t; H h[8];
    // registration
    memset(&t, 0, sizeof t);
    CHECK("a valid line registers", irq_register(&t, 3, h_noop, NULL) == 0, "returned non-zero");
    CHECK("the same line cannot be registered twice", irq_register(&t, 3, h_noop, NULL) == -1, "second registration was accepted");
    CHECK("line 8 and NULL handlers are rejected", irq_register(&t, 8, h_noop, NULL) == -1 && irq_register(&t, 100, h_noop, NULL) == -1 && irq_register(&t, 4, NULL, NULL) == -1 && t.isr[4] == NULL, "an invalid registration was accepted");

    // enable
    i_init(&i); i.enable = 0xA5;
    intc_enable(&i.bus, 1, 1); intc_enable(&i.bus, 7, 0); intc_enable(&i.bus, 0, 0);
    CHECK("intc_enable sets and clears one bit at a time", i.enable == 0x26u, "ENABLE is 0x%X, expected 0x26", i.enable);
    CHECK("intc_enable is one read and one write per call", i.enable_reads == 3 && i.enable_writes == 3, "%d reads, %d writes for 3 calls", i.enable_reads, i.enable_writes);

    // ordering and ctx
    i_init(&i); memset(&t, 0, sizeof t); g_n = 0;
    for (int l = 0; l < 8; l++) { memset(&h[l], 0, sizeof h[l]); h[l].intc = &i; h[l].line = l; irq_register(&t, l, h_run, &h[l]); }
    i.enable = 0xFF; i.pending = (1u << 5) | (1u << 1) | (1u << 3);
    int n = irq_dispatch(&i.bus, &t);
    CHECK("handlers run once each, lowest line first", n == 3 && g_n == 3 && g_order[0] == 1 && g_order[1] == 3 && g_order[2] == 5, "ran %d handlers, order %d %d %d", n, g_order[0], g_order[1], g_order[2]);
    CHECK("each handler receives its own context", h[1].calls == 1 && h[3].calls == 1 && h[5].calls == 1 && h[0].calls == 0, "context mix-up");
    CHECK("acks are single-bit writes for dispatched lines only", i.acks == 3 && i.ack_log[0] == 2 && i.ack_log[1] == 8 && i.ack_log[2] == 32, "%d acks: 0x%X 0x%X 0x%X", i.acks, i.ack_log[0], i.ack_log[1], i.ack_log[2]);
    CHECK("everything is clear afterwards", i.pending == 0, "pending 0x%X", i.pending);

    // masked line stays pending
    i_init(&i); memset(&t, 0, sizeof t); g_n = 0;
    for (int l = 0; l < 8; l++) { memset(&h[l], 0, sizeof h[l]); h[l].intc = &i; h[l].line = l; irq_register(&t, l, h_run, &h[l]); }
    i.enable = (1u << 2); i.pending = (1u << 2) | (1u << 6);
    n = irq_dispatch(&i.bus, &t);
    CHECK("a masked line is not run and not acknowledged", n == 1 && h[6].calls == 0 && (i.pending & (1u << 6)), "ran %d, pending 0x%X", n, i.pending);
    intc_enable(&i.bus, 6, 1);
    n = irq_dispatch(&i.bus, &t);
    CHECK("unmasking later delivers the saved event", n == 1 && h[6].calls == 1 && i.pending == 0, "ran %d, pending 0x%X", n, i.pending);

    // event during handler
    i_init(&i); memset(&t, 0, sizeof t); g_n = 0;
    for (int l = 0; l < 8; l++) { memset(&h[l], 0, sizeof h[l]); h[l].intc = &i; h[l].line = l; irq_register(&t, l, h_run, &h[l]); }
    h[2].refire = 2; i.enable = 0xFF; i.pending = 1u << 2;
    n = irq_dispatch(&i.bus, &t);
    CHECK("an event raised during its own handler is not lost", h[2].calls == 3 && n == 3 && i.pending == 0, "handler ran %d times, expected 3 (ack must come first)", h[2].calls);

    // stuck line
    i_init(&i); memset(&t, 0, sizeof t);
    memset(&h[0], 0, sizeof h[0]); h[0].intc = &i; h[0].line = 0;
    irq_register(&t, 0, h_stuck, &h[0]);
    i.enable = 1; i.pending = 1;
    n = irq_dispatch(&i.bus, &t);
    CHECK("a stuck line is limited to 16 passes", n == 16 && h[0].calls == 16, "ran %d times", h[0].calls);

    // spurious
    i_init(&i); memset(&t, 0, sizeof t);
    irq_register(&t, 0, h_noop, NULL);
    i.enable = 0x0F; i.pending = (1u << 0) | (1u << 3);
    n = irq_dispatch(&i.bus, &t);
    CHECK("a line with no handler is acknowledged and counted", n == 1 && t.spurious == 1 && i.pending == 0, "ran %d, spurious %u, pending 0x%X", n, t.spurious, i.pending);

    // quiet
    i_init(&i); i.enable = 0xFF;
    n = irq_dispatch(&i.bus, &t);
    CHECK("nothing pending: no acks, no handlers", n == 0 && i.acks == 0 && i.reads_p == 1, "ran %d, %d acks, %d PENDING reads", n, i.acks, i.reads_p);
    return 0;
}
