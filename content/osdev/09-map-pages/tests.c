void bm_exception(uint64_t vector, uint64_t error_code, uint64_t rip, uint64_t cr2);
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

static uint8_t page_a[4096] __attribute__((aligned(4096)));
static uint8_t page_b[4096] __attribute__((aligned(4096)));
static uint8_t page_c[4096] __attribute__((aligned(4096)));
static uint8_t page_d[4096] __attribute__((aligned(4096)));

#define P(x) ((uint64_t)(uintptr_t)(x))
#define V(addr) (*(volatile uint64_t *)(uintptr_t)(addr))

static volatile int faults;
static volatile uint64_t fault_addr, fault_err;
static uint64_t g_cr3, remap_target;

static void on_fault(uint64_t vector, uint64_t error_code, uint64_t rip) {
    if (vector != 14) bm_exception(vector, error_code, rip, 0);
    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    faults++;
    fault_addr = cr2;
    fault_err = error_code;
    map_page(g_cr3, cr2 & ~0xFFFULL, remap_target, PTE_WRITABLE);   // demand paging
}

void kmain(void) {
    uint64_t cr3 = read_cr3();
    g_cr3 = cr3;

    size_t before = frames_used;
    int r = map_page(cr3, 0x40000000, P(page_a), PTE_WRITABLE);
    CHECK("map_page maps a page above the first GiB", r == 0, "returned %d", r);
    CHECK("it allocated exactly the two missing tables (PD and PT)", frames_used - before == 2, "allocated %lu frames", (unsigned long)(frames_used - before));

    V(0x40000000) = 0xCAFEBABE12345678ULL;
    uint64_t seen = *(volatile uint64_t *)page_a;
    CHECK("a write through the new virtual address lands in the physical frame", seen == 0xCAFEBABE12345678ULL, "frame holds 0x%lx", seen);
    page_a[0x123] = 0x5A;
    CHECK("a read through the new virtual address sees the frame's data", (*(volatile uint8_t *)(uintptr_t)0x40000123) == 0x5A, "read 0x%x", *(volatile uint8_t *)(uintptr_t)0x40000123);
    CHECK("translate agrees with the hardware", translate(cr3, 0x40000123, NULL) == P(page_a) + 0x123, "translate gave 0x%lx", translate(cr3, 0x40000123, NULL));

    before = frames_used;
    r = map_page(cr3, 0x40001000, P(page_b), PTE_WRITABLE);
    CHECK("a neighbouring page reuses the existing tables", r == 0 && frames_used == before, "returned %d, allocated %lu frames", r, (unsigned long)(frames_used - before));
    V(0x40001000) = 0x1111222233334444ULL;
    CHECK("the second page is distinct memory", *(volatile uint64_t *)page_b == 0x1111222233334444ULL && V(0x40000000) == 0xCAFEBABE12345678ULL, "pages overlap");

    before = frames_used;
    r = map_page(cr3, 0x40200000, P(page_c), PTE_WRITABLE);
    CHECK("a page in the next 2 MiB needs one new page table", r == 0 && frames_used - before == 1, "returned %d, allocated %lu frames", r, (unsigned long)(frames_used - before));
    V(0x40200000) = 77;
    CHECK("and it works", *(volatile uint64_t *)page_c == 77, "frame holds %lu", *(volatile uint64_t *)page_c);

    CHECK("mapping an already-mapped page fails", map_page(cr3, 0x40001000, P(page_d), PTE_WRITABLE) == -1, "should return -1");
    CHECK("an unaligned virtual address fails", map_page(cr3, 0x40005001, P(page_d), 0) == -1, "should return -1");
    CHECK("an unaligned physical address fails", map_page(cr3, 0x40005000, P(page_d) + 8, 0) == -1, "should return -1");
    CHECK("a non-canonical virtual address fails", map_page(cr3, 0x0000800000000000ULL, P(page_d), 0) == -1, "should return -1");
    CHECK("mapping inside a 2 MiB huge page fails", map_page(cr3, 0x400000, P(page_d), 0) == -1, "should return -1");
    CHECK("mapping over an existing 4 KiB mapping of the kernel fails", map_page(cr3, 0x2000, P(page_d), 0) == -1, "should return -1");

    uint64_t perms;
    CHECK("a kernel-only writable mapping is writable but not user", map_page(cr3, 0x40004000, P(page_d), PTE_WRITABLE) == 0 && (translate(cr3, 0x40004000, &perms), perms == PTE_WRITABLE), "perms 0x%lx", perms);
    CHECK("a read-only mapping has no flags", map_page(cr3, 0x40006000, P(page_d), 0) == 0 && (translate(cr3, 0x40006000, &perms), perms == 0), "perms 0x%lx", perms);
    CHECK("PTE_NX is stored", map_page(cr3, 0x40007000, P(page_d), PTE_NX) == 0 && (translate(cr3, 0x40007000, &perms), perms == PTE_NX), "perms 0x%lx", perms);
    CHECK("a user mapping makes the whole path user-accessible", map_page(cr3, 0x40008000, P(page_d), PTE_USER | PTE_WRITABLE) == 0 && (translate(cr3, 0x40008000, &perms), perms == (PTE_USER | PTE_WRITABLE)), "perms 0x%lx", perms);
    CHECK("junk bits in flags are ignored", map_page(cr3, 0x40009000, P(page_d), PTE_HUGE | PTE_PRESENT | 0x1F0) == 0 && (translate(cr3, 0x40009000, &perms), perms == 0) && translate(cr3, 0x40009000, NULL) == P(page_d), "perms 0x%lx", perms);
    CHECK("the kernel mappings keep working after all this", translate(cr3, P(page_a), NULL) == P(page_a), "kernel mapping broken");

    // ---- unmapping, TLB and demand paging ----
    CHECK("unmap_page on an unmapped address fails", unmap_page(cr3, 0x50000000) == -1, "should return -1");
    CHECK("unmap_page on an unaligned address fails", unmap_page(cr3, 0x40001008) == -1, "should return -1");
    CHECK("unmap_page on a 2 MiB huge page fails", unmap_page(cr3, 0x400000) == -1, "should return -1");

    (void)V(0x40001000);   // make sure the translation is cached in the TLB
    r = unmap_page(cr3, 0x40001000);
    CHECK("unmap_page returns 0 for a mapped page", r == 0, "returned %d", r);
    CHECK("the page is gone from the tables", translate(cr3, 0x40001000, NULL) == TRANSLATE_FAULT, "translate still finds it");
    CHECK("a second unmap fails", unmap_page(cr3, 0x40001000) == -1, "should return -1");

    remap_target = P(page_d);
    page_d[0] = 0xEE;
    faults = 0;
    bm_exception_hook = on_fault;
    uint64_t got = V(0x40001000);
    bm_exception_hook = 0;
    CHECK("touching the unmapped page raises exactly one page fault", faults == 1,
          "%d page faults: after unmap_page the stale TLB entry still worked. Flush it with invlpg", faults);
    CHECK("CR2 holds the faulting address and the error says not-present", fault_addr == 0x40001000 && (fault_err & 1) == 0, "cr2 0x%lx, error 0x%lx", fault_addr, fault_err);
    CHECK("after the handler maps a page the access succeeds", (got & 0xFF) == 0xEE, "read 0x%lx", got);
}
