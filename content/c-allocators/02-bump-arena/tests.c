static unsigned rng(unsigned *s) { *s = *s * 1103515245u + 12345u; return *s >> 16; }

#define CAP 600
static _Alignas(64) unsigned char store[256 + CAP];

// Reference model: tracks only integers (offsets), computed from the real base address.
typedef struct { size_t off; } Model;
static size_t model_alloc(Model *m, uintptr_t base, size_t cap, size_t size, size_t align, int *ok) {
    *ok = 0;
    if (size == 0 || align == 0 || (align & (align - 1))) return 0;
    uintptr_t addr = base + m->off;
    size_t pad = 0;
    while ((addr + pad) % align) pad++;
    if (pad + size < pad || m->off + pad + size > cap || m->off + pad + size < m->off) return 0;
    *ok = 1;
    size_t at = m->off + pad;
    m->off += pad + size;
    return at;
}

typedef struct { unsigned char *p; size_t size; unsigned char tag; } Live;

// Returns an empty string on success or a description of the first divergence.
static const char *run_seed(unsigned seed, size_t skew, char *msg, size_t msglen) {
    unsigned s = seed;
    unsigned char *buf = store + skew;
    Arena a;
    Model m = { 0 };
    arena_init(&a, buf, CAP);
    Live live[200];
    int nlive = 0;
    size_t marks[8], mark_live[8];
    int nmarks = 0;
    for (int step = 0; step < 300; step++) {
        unsigned r = rng(&s) % 100;
        if (r < 70) {
            static const size_t al[] = { 1, 2, 4, 8, 16, 32, 64 };
            size_t align = al[rng(&s) % 7];
            size_t size = rng(&s) % 40;
            if (rng(&s) % 20 == 0) size = CAP - (rng(&s) % 5);       // nearly everything
            if (rng(&s) % 50 == 0) size = SIZE_MAX - (rng(&s) % 4);   // overflow bait
            if (rng(&s) % 60 == 0) align = 12;                       // not a power of two
            size_t before = a.off;
            int ok;
            size_t at = model_alloc(&m, (uintptr_t)buf, CAP, size, align, &ok);
            void *p = arena_alloc(&a, size, align);
            if (!ok) {
                if (p) { snprintf(msg, msglen, "step %d: arena_alloc(%zu, %zu) returned a pointer, expected NULL", step, size, align); return msg; }
                if (a.off != before) { snprintf(msg, msglen, "step %d: a failed allocation changed off from %zu to %zu", step, before, a.off); return msg; }
                continue;
            }
            if (!p) { snprintf(msg, msglen, "step %d: arena_alloc(%zu, %zu) returned NULL with %zu bytes free", step, size, align, CAP - before); return msg; }
            if ((unsigned char *)p != buf + at) { snprintf(msg, msglen, "step %d: arena_alloc(%zu, %zu) returned base+%td, expected base+%zu", step, size, align, (unsigned char *)p - buf, at); return msg; }
            if (((uintptr_t)p & (align - 1)) != 0) { snprintf(msg, msglen, "step %d: address %p is not %zu-aligned", step, p, align); return msg; }
            if (arena_used(&a) != m.off) { snprintf(msg, msglen, "step %d: arena_used is %zu, expected %zu", step, arena_used(&a), m.off); return msg; }
            if (nlive < 200) {
                live[nlive].p = p; live[nlive].size = size; live[nlive].tag = (unsigned char)(step * 7 + 1);
                memset(p, live[nlive].tag, size);
                nlive++;
            }
        } else if (r < 80 && nmarks < 8) {
            marks[nmarks] = arena_mark(&a);
            mark_live[nmarks] = nlive;
            if (marks[nmarks] != m.off) { snprintf(msg, msglen, "step %d: arena_mark is %zu, expected %zu", step, marks[nmarks], m.off); return msg; }
            nmarks++;
        } else if (r < 90 && nmarks > 0) {
            nmarks--;
            arena_rewind(&a, marks[nmarks]);
            m.off = marks[nmarks];
            nlive = (int)mark_live[nmarks];
            if (arena_used(&a) != m.off) { snprintf(msg, msglen, "step %d: after rewind used is %zu, expected %zu", step, arena_used(&a), m.off); return msg; }
        } else if (r < 93) {
            size_t before = a.off;
            arena_rewind(&a, before + 1 + rng(&s) % 50);
            if (a.off != before) { snprintf(msg, msglen, "step %d: rewinding to a mark ahead of off changed off to %zu (was %zu)", step, a.off, before); return msg; }
        } else if (r < 95) {
            arena_reset(&a);
            m.off = 0; nlive = 0; nmarks = 0;
            if (arena_used(&a) != 0) { snprintf(msg, msglen, "step %d: used is %zu after reset", step, arena_used(&a)); return msg; }
        }
        // Independent overlap/content check: every live block still holds its own tag.
        for (int i = 0; i < nlive; i++) {
            for (size_t k = 0; k < live[i].size; k++) {
                if (live[i].p[k] != live[i].tag) { snprintf(msg, msglen, "step %d: block %d was overwritten by a later allocation (overlap)", step, i); return msg; }
            }
        }
    }
    msg[0] = 0;
    return msg;
}

int main(void) {
    Arena a;
    arena_init(&a, store, 64);
    CHECK("a fresh arena has used 0", arena_used(&a) == 0, "used is %zu", arena_used(&a));
    unsigned char *p = arena_alloc(&a, 10, 1);
    CHECK("the first allocation starts at the base", p == store, "got %p, base %p", (void *)p, (void *)store);
    unsigned char *q = arena_alloc(&a, 8, 8);
    CHECK("the next 8-aligned block skips the padding", q == store + 16 && arena_used(&a) == 24, "q is base+%td, used %zu (expected base+16 and 24)", q ? q - store : -1, arena_used(&a));
    CHECK("an impossible size fails and changes nothing", arena_alloc(&a, 100, 1) == NULL && arena_used(&a) == 24, "used is %zu", arena_used(&a));
    CHECK("an enormous size does not wrap around", arena_alloc(&a, SIZE_MAX, 1) == NULL && arena_alloc(&a, SIZE_MAX - 7, 8) == NULL && arena_used(&a) == 24, "used is %zu", arena_used(&a));
    CHECK("zero bytes and bad alignments return NULL", arena_alloc(&a, 0, 1) == NULL && arena_alloc(&a, 4, 0) == NULL && arena_alloc(&a, 4, 3) == NULL && arena_used(&a) == 24, "used is %zu", arena_used(&a));
    CHECK("a request that fills the arena exactly succeeds", arena_alloc(&a, 40, 1) == store + 24 && arena_used(&a) == 64, "used is %zu", arena_used(&a));
    CHECK("one more byte fails", arena_alloc(&a, 1, 1) == NULL, "arena handed out memory past its capacity");
    arena_reset(&a);
    CHECK("reset makes the whole arena available again", arena_used(&a) == 0 && arena_alloc(&a, 64, 64) == store, "used is %zu", arena_used(&a));

    // Padding needed to reach the alignment can itself exceed the capacity.
    Arena t;
    arena_init(&t, store + 1, 10);
    CHECK("padding that does not fit fails cleanly", arena_alloc(&t, 1, 64) == NULL && arena_used(&t) == 0, "used is %zu", arena_used(&t));

    // Buffers that start at awkward addresses.
    static const size_t skews[] = { 0, 1, 3, 5, 7, 9, 13, 24 };
    char msg[200];
    for (unsigned k = 0; k < 8; k++) {
        char name[80];
        snprintf(name, sizeof name, "seeded random operations, buffer misaligned by %zu", skews[k]);
        const char *bad = run_seed(1000u + 17u * k, skews[k], msg, sizeof msg);
        CHECK(name, bad[0] == 0, "%s", bad);
    }
    return 0;
}
