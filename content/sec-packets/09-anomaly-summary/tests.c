static uint32_t st = 4242;
static uint32_t rnd(void) { st = st * 1664525u + 1013904223u; return st >> 8; }

/* Independent reference: for each source, count distinct qualifying ports. Sources sorted ascending. */
static size_t reference(const Flow *f, size_t n, size_t threshold, uint32_t *out)
{
    size_t count = 0;
    if (!threshold)
        return 0;
    for (size_t i = 0; i < n; i++) {
        size_t ports = 0, first = 1;
        static uint8_t seen[65536];
        for (size_t j = 0; j < i; j++)
            if (f[j].src == f[i].src) first = 0;
        if (!first)
            continue;
        memset(seen, 0, sizeof seen);
        for (size_t j = 0; j < n; j++)
            if (f[j].src == f[i].src && (f[j].flags & 0x12) == 0x02 && !seen[f[j].port]) { seen[f[j].port] = 1; ports++; }
        if (ports >= threshold) {
            size_t at = count++;
            while (at && out[at - 1] > f[i].src) { out[at] = out[at - 1]; at--; }
            out[at] = f[i].src;
        }
    }
    return count;
}

int main(void)
{
    uint32_t out[130], want[130];

    /* Fixed scenario with one source per rule. */
    {
        const uint32_t A = 0xc000020au, B = 0xc6336407u, C = 0xcb007109u;
        Flow f[] = {
            {A, 80, 0x02}, {A, 80, 0x02}, {A, 443, 0x02}, {A, 22, 0x12},
            {B, 22, 0x02}, {B, 23, 0x02}, {B, 25, 0x42}, {B, 80, 0x10}, {B, 81, 0x04},
            {C, 1, 0x02}, {C, 2, 0x02}, {C, 3, 0x03}, {C, 4, 0x02}, {C, 5, 0x02}, {C, 6, 0x12},
        };
        size_t n = sizeof f / sizeof f[0];
        size_t two = scan_sources(f, n, 2, out, 8);
        CHECK("distinct ports reach the threshold", two == 3 && out[0] == A && out[1] == B && out[2] == C, "threshold 2: returned %zu (want 3: A, B, C in ascending order)", two);
        size_t three = scan_sources(f, n, 3, out, 8);
        CHECK("repeats and non-SYN flows do not count", three == 2 && out[0] == B && out[1] == C, "threshold 3: returned %zu (want B and C; A has two distinct SYN ports, B has 22/23/25)", three);
        size_t five = scan_sources(f, n, 5, out, 8);
        CHECK("only the busiest source remains", five == 1 && out[0] == C, "threshold 5: returned %zu (want only C, which has ports 1, 2, 3, 4, 5)", five);
        CHECK("threshold above everything", scan_sources(f, n, 6, out, 8) == 0, "no source has six distinct SYN ports");
    }

    /* Flag semantics. */
    {
        Flow f[] = {{1, 1, 0x02}, {1, 2, 0x42}, {1, 3, 0xc2}, {1, 4, 0x12}, {1, 5, 0x10}, {1, 6, 0x00}, {1, 7, 0x04}, {1, 8, 0x1a}};
        size_t got = scan_sources(f, 8, 1, out, 4);
        size_t three = scan_sources(f, 8, 3, out, 4);
        size_t four = scan_sources(f, 8, 4, out, 4);
        CHECK("flag mask", got == 1 && three == 1 && four == 0, "ports 1, 2 and 3 qualify (0x02, 0x42, 0xc2) but SYN-ACK 0x12, ACK, no flags, RST and 0x1a do not: threshold 3 gave %zu, threshold 4 gave %zu (want 1 and 0)", three, four);
    }

    /* Same port from two sources, and a port repeated many times. */
    {
        Flow f[40];
        size_t n = 0;
        for (int i = 0; i < 30; i++) f[n++] = (Flow){7, 80, 0x02};
        f[n++] = (Flow){8, 80, 0x02};
        f[n++] = (Flow){8, 81, 0x02};
        size_t got = scan_sources(f, n, 2, out, 4);
        CHECK("repetition is not breadth", got == 1 && out[0] == 8, "source 7 sent 30 SYNs to one port and must not qualify at threshold 2; returned %zu", got);
    }

    /* High-bit addresses sort as unsigned numbers. */
    {
        Flow f[] = {{0xffffff01u, 1, 2}, {0xffffff01u, 2, 2}, {0x00000a01u, 1, 2}, {0x00000a01u, 2, 2}, {0x80000000u, 1, 2}, {0x80000000u, 2, 2}};
        size_t got = scan_sources(f, 6, 2, out, 8);
        CHECK("addresses sort as unsigned values", got == 3 && out[0] == 0x00000a01u && out[1] == 0x80000000u && out[2] == 0xffffff01u, "returned %zu: 0x%08x 0x%08x 0x%08x (want ascending unsigned order)", got, out[0], out[1], out[2]);
    }

    /* max keeps the lowest addresses and writes nothing past itself. */
    {
        Flow f[] = {{30, 1, 2}, {10, 1, 2}, {20, 1, 2}, {40, 1, 2}};
        for (int i = 0; i < 6; i++) out[i] = 0xdeadbeefu;
        size_t two = scan_sources(f, 4, 1, out, 2);
        CHECK("max keeps the lowest addresses", two == 2 && out[0] == 10 && out[1] == 20 && out[2] == 0xdeadbeefu, "max 2 of four qualifying sources returned %zu: %u %u, next slot 0x%x (must be untouched)", two, out[0], out[1], out[2]);
        size_t all = scan_sources(f, 4, 1, out, 4);
        CHECK("max equal to the result size", all == 4 && out[3] == 40, "max 4 should return 4 sorted values, got %zu", all);
    }

    /* Disabled and empty inputs write nothing. */
    {
        Flow f[] = {{1, 1, 2}};
        out[0] = 0xdeadbeefu;
        CHECK("disabled, empty or zero capacity", scan_sources(f, 1, 0, out, 4) == 0 && scan_sources(NULL, 0, 1, out, 4) == 0 && scan_sources(f, 1, 1, out, 0) == 0 && out[0] == 0xdeadbeefu, "threshold 0, n 0 and max 0 must return 0 without writing");
    }

    /* Generated flow lists against the reference. */
    for (int round = 0; round < 40; round++) {
        Flow f[128];
        size_t n = 1 + rnd() % 128;
        size_t sources = 1 + rnd() % 6;
        static const uint32_t pool[] = {0xc0000201u, 0x0a000001u, 0xffffff00u, 0x80000001u, 0xc6336407u, 0x00000001u};
        static const uint8_t fl[] = {0x02, 0x02, 0x02, 0x12, 0x10, 0x04, 0x42, 0x00};
        for (size_t i = 0; i < n; i++)
            f[i] = (Flow){pool[rnd() % sources], (uint16_t)(rnd() % 12), fl[rnd() % 8]};
        size_t threshold = 1 + rnd() % 8;
        size_t max = 1 + rnd() % 8;
        size_t all = reference(f, n, threshold, want);
        size_t w = all < max ? all : max;
        size_t got = scan_sources(f, n, threshold, out, max);
        CHECK("generated flow lists", got == w && !memcmp(out, want, w * sizeof *out), "round %d (n=%zu, threshold=%zu, max=%zu): returned %zu, want %zu", round, n, threshold, max, got, w);
    }
    return 0;
}
