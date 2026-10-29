#define A_BASE 0x80000000ull
#define A_PAGES 16
#define B_BASE 0x80010000ull
#define B_PAGES 4
#define C_BASE 0x90000000ull
#define C_PAGES 10240
#define D_BASE (C_BASE + C_PAGES * 4096ull)

// The test walks the live tables itself to look at the leaf entry for an address.
static uint64_t t_pte(uint64_t virt) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *t = (uint64_t *)(uintptr_t)(cr3 & 0x000FFFFFFFFFF000ull);
    for (int level = 4; level > 1; level--) {
        uint64_t e = t[(virt >> (12 + 9 * (level - 1))) & 511];
        if (!(e & 1)) return 0;
        if (e & 0x80) return e;
        t = (uint64_t *)(uintptr_t)(e & 0x000FFFFFFFFFF000ull);
    }
    return t[(virt >> 12) & 511];
}

void kmain(void) {
    memset((void *)0x1000000, 0xAA, 0x400000);   // dirty the first 4 MiB of the frame pool
    vm_init();

    CHECK("an unaligned start is rejected", vm_region_add(A_BASE + 8, 4096, 1) == -1, "accepted an unaligned start");
    CHECK("an unaligned length is rejected", vm_region_add(A_BASE, 4097, 1) == -1, "accepted an unaligned length");
    CHECK("a zero length is rejected", vm_region_add(A_BASE, 0, 1) == -1, "accepted a zero-length region");
    CHECK("a region inside the identity-mapped first GiB is rejected", vm_region_add(0x200000, 4096, 1) == -1, "accepted an address that is already mapped");
    CHECK("a range that wraps around 2^64 is rejected", vm_region_add(0xFFFFFFFFFFFFF000ull, 0x2000, 1) == -1 && vm_region_add(0x00007FFFFFFFF000ull, 0x3000, 1) == -1, "accepted a range beyond the user half");
    CHECK("a valid region is accepted", vm_region_add(A_BASE, A_PAGES * 4096ull, 1) == 0, "rejected a valid region");
    CHECK("an overlapping region is rejected", vm_region_add(A_BASE + 8 * 4096, 4 * 4096, 1) == -1 && vm_region_add(A_BASE - 4096, 2 * 4096, 1) == -1, "accepted an overlap");
    CHECK("an adjacent read-only region is accepted", vm_region_add(B_BASE, B_PAGES * 4096ull, 0) == 0, "rejected an adjacent region");
    CHECK("a large region is accepted", vm_region_add(C_BASE, C_PAGES * 4096ull, 1) == 0, "rejected region C");
    CHECK("a fourth region is accepted", vm_region_add(D_BASE, 2 * 4096, 1) == 0, "rejected region D");
    CHECK("a fifth region is rejected", vm_region_add(0xA0000000ull, 4096, 1) == -1, "accepted more than four regions");

    uint64_t frames_before = 0;
    (void)frames_before;
    CHECK("pf_handle refuses an address outside every region", pf_handle(0x70000000ull, 2) == 0 && pages_faulted == 0, "accepted a wild address");
    CHECK("pf_handle refuses a protection fault (page present)", pf_handle(A_BASE, 3) == 0 && pages_faulted == 0, "mapped over a present page");
    CHECK("pf_handle refuses an instruction fetch", pf_handle(A_BASE, 0x10) == 0 && pages_faulted == 0, "accepted a fetch");
    CHECK("pf_handle refuses a first-touch write to a read-only region", pf_handle(B_BASE, 2) == 0 && pages_faulted == 0 && t_pte(B_BASE) == 0, "mapped a page for a forbidden write");

    // Real faults.
    volatile uint8_t *a = (volatile uint8_t *)A_BASE;
    uint8_t v = a[100];
    CHECK("a read of a lazy page succeeds and the page is zero-filled (not the 0xAA the frame held)", v == 0 && a[4095] == 0 && a[0] == 0, "read 0x%02x", v);
    CHECK("the first touch is one page fault", pages_faulted == 1, "pages_faulted is %lu", (unsigned long)pages_faulted);
    uint64_t pte0 = t_pte(A_BASE);
    CHECK("the page is present and writable", (pte0 & 3) == 3, "PTE is 0x%lx", (unsigned long)pte0);
    a[50] = 7;
    (void)a[4000];
    CHECK("touching the same page again causes no new fault", pages_faulted == 1 && a[50] == 7, "pages_faulted is %lu", (unsigned long)pages_faulted);

    a[4096] = 0x42;
    CHECK("a write to the next page faults once more", pages_faulted == 2 && a[4096] == 0x42, "pages_faulted is %lu", (unsigned long)pages_faulted);
    CHECK("the page is at the faulting page's address, not the region's start", t_pte(A_BASE + 4096) != 0 && t_pte(A_BASE + 4096) != pte0, "second PTE is 0x%lx", (unsigned long)t_pte(A_BASE + 4096));

    for (int i = A_PAGES - 1; i >= 2; i--) {
        volatile uint64_t *w = (volatile uint64_t *)(A_BASE + (uint64_t)i * 4096 + 8);
        *w = 0x1000 + (uint64_t)i;
    }
    int intact = 1, zero = 1;
    uint64_t frames[A_PAGES];
    for (int i = 0; i < A_PAGES; i++) {
        if (i >= 2 && *(volatile uint64_t *)(A_BASE + (uint64_t)i * 4096 + 8) != 0x1000 + (uint64_t)i) intact = 0;
        if (*(volatile uint8_t *)(A_BASE + (uint64_t)i * 4096 + 2000) != 0) zero = 0;
        frames[i] = t_pte(A_BASE + (uint64_t)i * 4096) & 0x000FFFFFFFFFF000ull;
    }
    int distinct = 1;
    for (int i = 0; i < A_PAGES; i++) {
        if (frames[i] < 0x1000000 || frames[i] >= 0x3000000) distinct = 0;
        for (int j = 0; j < i; j++) if (frames[i] == frames[j]) distinct = 0;
    }
    CHECK("pages touched in reverse order each keep their own data", intact, "a page lost its value");
    CHECK("every page is zero-filled before first use", zero, "a page contained stale data");
    CHECK("each page gets its own frame from the pool", distinct, "two pages share a frame or a frame is outside the pool");
    CHECK("the whole region is now resident: one fault per page", pages_faulted == A_PAGES, "pages_faulted is %lu, expected %d", (unsigned long)pages_faulted, A_PAGES);

    volatile uint8_t *b = (volatile uint8_t *)B_BASE;
    uint8_t bv = b[10];
    uint64_t bpte = t_pte(B_BASE);
    CHECK("a read of a read-only lazy region works and is zero", bv == 0 && pages_faulted == A_PAGES + 1, "read 0x%02x, pages_faulted %lu", bv, (unsigned long)pages_faulted);
    CHECK("the read-only page is mapped without the writable bit", (bpte & 1) == 1 && (bpte & 2) == 0, "PTE is 0x%lx", (unsigned long)bpte);
    CHECK("a write to the now-present read-only page is a protection fault and is refused", pf_handle(B_BASE, 3) == 0 && (t_pte(B_BASE) & 2) == 0, "the page became writable");

    uint64_t before = pages_faulted;
    volatile uint8_t *d = (volatile uint8_t *)(D_BASE + 4096);
    *d = 9;
    CHECK("the last page of the last region is reachable", *d == 9 && pages_faulted == before + 1, "pages_faulted is %lu", (unsigned long)pages_faulted);

    // Out of memory: touch region C until the frame pool is empty.
    uint64_t ok = 0;
    int refused = 0;
    for (uint64_t i = 0; i < C_PAGES; i++) {
        if (pf_handle(C_BASE + i * 4096, 2)) ok++;
        else { refused = 1; break; }
    }
    CHECK("when frames run out pf_handle refuses instead of mapping garbage", refused && ok > 6000 && ok < C_PAGES, "accepted %lu pages, refused: %d", (unsigned long)ok, refused);
    CHECK("pages_faulted counts only accepted faults", pages_faulted == before + 1 + ok, "pages_faulted is %lu, expected %lu", (unsigned long)pages_faulted, (unsigned long)(before + 1 + ok));
    CHECK("the refused page was left unmapped", t_pte(C_BASE + ok * 4096) == 0, "PTE is 0x%lx", (unsigned long)t_pte(C_BASE + ok * 4096));

    vm_init();
    CHECK("vm_init forgets the regions and resets the counter", pages_faulted == 0 && vm_region_add(A_BASE, 4096, 1) == 0, "pages_faulted is %lu", (unsigned long)pages_faulted);
}
