static uint64_t pml4[512] __attribute__((aligned(4096)));
static uint64_t pdpt[512] __attribute__((aligned(4096)));
static uint64_t pd[512] __attribute__((aligned(4096)));
static uint64_t pt[512] __attribute__((aligned(4096)));

#define VIRT 0xFFFF800000201234ULL   // PML4 256, PDPT 0, PD 1, PT 1, offset 0x234
#define TABLE(t) ((uint64_t)(uintptr_t)(t))

static void build(void) {
    memset(pml4, 0, sizeof pml4); memset(pdpt, 0, sizeof pdpt); memset(pd, 0, sizeof pd); memset(pt, 0, sizeof pt);
    pml4[256] = TABLE(pdpt) | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    pdpt[0] = TABLE(pd) | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    pd[1] = TABLE(pt) | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    pt[1] = 0x5000 | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
}

void kmain(void) {
    CHECK("pt_index extracts the PML4 index from bits 47:39", pt_index(VIRT, 4) == 256, "got %u", pt_index(VIRT, 4));
    CHECK("pt_index extracts PDPT, PD and PT indices", pt_index(VIRT, 3) == 0 && pt_index(VIRT, 2) == 1 && pt_index(VIRT, 1) == 1,
          "got %u %u %u", pt_index(VIRT, 3), pt_index(VIRT, 2), pt_index(VIRT, 1));
    CHECK("pt_index gives 255 and 511 at the top of the lower half", pt_index(0x00007FFFFFFFF000ULL, 4) == 255 && pt_index(0x00007FFFFFFFF000ULL, 1) == 511,
          "got %u and %u", pt_index(0x00007FFFFFFFF000ULL, 4), pt_index(0x00007FFFFFFFF000ULL, 1));
    CHECK("pt_index ignores the sign extension and the page offset", pt_index(0xFFFFFFFFFFFFFFFFULL, 4) == 511 && pt_index(0x0000000000000FFFULL, 1) == 0,
          "got %u and %u", pt_index(0xFFFFFFFFFFFFFFFFULL, 4), pt_index(0x0000000000000FFFULL, 1));

    build();
    uint64_t perms = 0xFF;
    uint64_t p = translate(TABLE(pml4), VIRT, &perms);
    CHECK("translate follows four levels to the frame plus offset", p == 0x5234, "got 0x%lx (expected 0x5234)", p);
    CHECK("translate reports writable and user when every level allows them", perms == (PTE_WRITABLE | PTE_USER), "perms 0x%lx", perms);
    CHECK("translate accepts a NULL perms pointer", translate(TABLE(pml4), VIRT, NULL) == 0x5234, "should still translate");
    CHECK("translate ignores the low 12 bits of cr3", translate(TABLE(pml4) | 0x7FF, VIRT, NULL) == 0x5234, "cr3 flag bits leaked into the address");

    pd[1] &= ~PTE_USER;
    translate(TABLE(pml4), VIRT, &perms);
    CHECK("one level without the user bit makes the mapping kernel-only", perms == PTE_WRITABLE, "perms 0x%lx", perms);
    build(); pt[1] &= ~PTE_WRITABLE;
    translate(TABLE(pml4), VIRT, &perms);
    CHECK("one read-only level makes the whole mapping read-only", perms == PTE_USER, "perms 0x%lx", perms);
    build(); pdpt[0] |= PTE_NX;
    translate(TABLE(pml4), VIRT, &perms);
    CHECK("NX at any level makes the page non-executable", perms == (PTE_WRITABLE | PTE_USER | PTE_NX), "perms 0x%lx", perms);

    build(); pml4[256] = 0;
    CHECK("a missing PML4 entry faults", translate(TABLE(pml4), VIRT, NULL) == TRANSLATE_FAULT, "should fault");
    build(); pdpt[0] = 0;
    CHECK("a missing PDPT entry faults", translate(TABLE(pml4), VIRT, NULL) == TRANSLATE_FAULT, "should fault");
    build(); pd[1] = 0;
    CHECK("a missing PD entry faults", translate(TABLE(pml4), VIRT, NULL) == TRANSLATE_FAULT, "should fault");
    build(); pt[1] = 0x5000 | PTE_WRITABLE;
    CHECK("an entry without the present bit faults, whatever else is set", translate(TABLE(pml4), VIRT, NULL) == TRANSLATE_FAULT, "should fault");

    build();
    CHECK("a non-canonical address faults", translate(TABLE(pml4), 0x0000800000000000ULL, NULL) == TRANSLATE_FAULT && translate(TABLE(pml4), 0x1234567812345678ULL, NULL) == TRANSLATE_FAULT, "should fault");

    build(); pd[2] = 0x40000000ULL | PTE_PRESENT | PTE_WRITABLE | PTE_HUGE;
    uint64_t v2m = 0xFFFF800000400000ULL + 0x1ABCD;
    p = translate(TABLE(pml4), v2m, &perms);
    CHECK("a 2 MiB page keeps the low 21 bits of the address as the offset", p == 0x4001ABCD, "got 0x%lx (expected 0x4001ABCD)", p);
    CHECK("a 2 MiB page stops the walk: its perms come from three levels", perms == PTE_WRITABLE, "perms 0x%lx", perms);

    build(); pdpt[3] = 0x80000000ULL | PTE_PRESENT | PTE_HUGE | PTE_USER;
    uint64_t v1g = 0xFFFF800000000000ULL | (3ULL << 30) | 0x123456;
    p = translate(TABLE(pml4), v1g, NULL);
    CHECK("a 1 GiB page keeps the low 30 bits of the address as the offset", p == 0x80123456, "got 0x%lx (expected 0x80123456)", p);

    // ---- the machine's real boot tables, which the CPU is using right now ----
    uint64_t cr3 = read_cr3();
    static uint64_t probe;
    uint64_t self = (uint64_t)(uintptr_t)&probe;
    CHECK("the kernel is identity-mapped: a static variable translates to itself", translate(cr3, self, NULL) == self, "got 0x%lx for 0x%lx", translate(cr3, self, NULL), self);
    CHECK("an address in the first 2 MiB (4 KiB pages) translates to itself", translate(cr3, 0x1234, NULL) == 0x1234, "got 0x%lx", translate(cr3, 0x1234, NULL));
    CHECK("an address in a 2 MiB page translates to itself", translate(cr3, 0x20F123, NULL) == 0x20F123, "got 0x%lx", translate(cr3, 0x20F123, NULL));
    CHECK("page 0 is not mapped, so a null pointer would fault", translate(cr3, 0, NULL) == TRANSLATE_FAULT, "page 0 should be unmapped");
    CHECK("addresses above the first GiB are not mapped", translate(cr3, 0x40000000ULL, NULL) == TRANSLATE_FAULT, "should fault");
}
