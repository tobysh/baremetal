void bm_exception(uint64_t vector, uint64_t error_code, uint64_t rip, uint64_t cr2);
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

static volatile int faults;
static volatile uint64_t fault_cr2, fault_err;
volatile uint64_t marker = 0x600DF00D;

// Demand paging: a fault on an unmapped page maps a fresh frame there in the space that is loaded now.
static void on_fault(uint64_t vector, uint64_t error_code, uint64_t rip) {
    if (vector != 14) bm_exception(vector, error_code, rip, 0);
    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    faults++; fault_cr2 = cr2; fault_err = error_code;
    map_page(current_as, cr2 & ~4095ULL, frame_alloc(), PTE_W);
}

#define V(a) (*(volatile uint64_t *)(uintptr_t)(a))
#define P(a) (*(volatile uint64_t *)(uintptr_t)(a))
static uint64_t cr3_now(void) { uint64_t c; __asm__ volatile("mov %%cr3, %0" : "=r"(c)); return c & ADDR_MASK; }

void kmain(void) {
    aspace_boot();
    bm_exception_hook = on_fault;
    const uint64_t va = USER_BASE + 0x10000, vb = va + 4096, vc = va + 0x200000;

    uint64_t A = aspace_create();
    CHECK("aspace_create returns a frame and uses exactly one", A != 0 && (A & 4095) == 0 && frames_in_use == 1, "A=0x%lx, frames in use %lu", A, (unsigned long)frames_in_use);
    CHECK("the new PML4 is not the kernel's", A != kernel_pml4, "A equals kernel_pml4");
    uint64_t *a = (uint64_t *)(uintptr_t)A, *k = (uint64_t *)(uintptr_t)kernel_pml4;
    CHECK("PML4 entry 0 is shared with the kernel", A && a[0] == k[0] && (a[0] & 1), "A[0]=0x%lx kernel[0]=0x%lx", A ? a[0] : 0, k[0]);
    int empty = A != 0;
    for (int i = 1; i < 512 && A; i++) if (a[i]) empty = 0;
    CHECK("all user entries start out empty", empty, "an entry above 0 is not zero");

    uint64_t pa = aspace_alloc_page(A, va, 1);
    CHECK("the first user page allocates a PDPT, PD, PT and the page itself", pa != 0 && frames_in_use == 5, "pa=0x%lx, frames in use %lu (expected 5)", pa, (unsigned long)frames_in_use);
    uint64_t *leaf = leaf_pte(A, va);
    CHECK("the leaf entry points at the frame and is present, writable and user", leaf && (*leaf & ADDR_MASK) == pa && (*leaf & 7) == 7, "leaf=0x%lx", leaf ? *leaf : 0);
    int u_all = A != 0 && (a[1] & PTE_U) && (a[1] & PTE_P);
    if (u_all) { uint64_t *pdpt = (uint64_t *)(uintptr_t)(a[1] & ADDR_MASK); u_all = (pdpt[0] & PTE_U) != 0; }
    CHECK("the upper levels of the walk have the user bit too", u_all, "a table entry above the leaf lacks the U bit");
    CHECK("the kernel's entry 0 still has no user bit", (a[0] & PTE_U) == 0, "A[0]=0x%lx", a[0]);

    uint64_t pb = aspace_alloc_page(A, vb, 0);
    leaf = leaf_pte(A, vb);
    CHECK("a read-only user page: user bit set, writable bit clear", pb != 0 && leaf && (*leaf & 7) == 5, "leaf=0x%lx", leaf ? *leaf : 0);
    CHECK("a neighbouring page needs only its own frame", frames_in_use == 6, "frames in use %lu (expected 6)", (unsigned long)frames_in_use);
    uint64_t pc = aspace_alloc_page(A, vc, 1);
    CHECK("a page 2 MiB further needs one more page table", pc != 0 && frames_in_use == 8, "frames in use %lu (expected 8)", (unsigned long)frames_in_use);

    size_t before = frames_in_use;
    CHECK("an address in the kernel half is refused", aspace_alloc_page(A, 0x400000, 1) == 0, "mapped an address below USER_BASE");
    CHECK("an unaligned address is refused", aspace_alloc_page(A, va + 0x10000 + 8, 1) == 0, "mapped an unaligned address");
    CHECK("an address past the canonical half is refused", aspace_alloc_page(A, USER_END, 1) == 0, "mapped USER_END");
    CHECK("mapping an address twice is refused", aspace_alloc_page(A, va, 1) == 0, "mapped an address twice");
    CHECK("refused requests leak no frames", frames_in_use == before, "frames in use %lu, was %lu", (unsigned long)frames_in_use, (unsigned long)before);

    uint64_t B = aspace_create();
    uint64_t pbb = aspace_alloc_page(B, va, 1);
    aspace_switch(A);
    CHECK("aspace_switch loads cr3", cr3_now() == A && current_as == A, "cr3=0x%lx current_as=0x%lx", cr3_now(), current_as);
    CHECK("kernel memory stays visible after the switch", marker == 0x600DF00D, "the kernel's data is gone");
    V(va) = 0x1111;
    aspace_switch(B);
    CHECK("the same virtual address now reaches a different frame", cr3_now() == B && V(va) == 0, "read 0x%lx under B", V(va));
    V(va) = 0x2222;
    CHECK("each space wrote into its own frame", P(pa) == 0x1111 && P(pbb) == 0x2222, "frames hold 0x%lx and 0x%lx", P(pa), P(pbb));
    aspace_switch(A);
    CHECK("switching back shows A's data again", V(va) == 0x1111, "read 0x%lx", V(va));

    aspace_switch(B);
    faults = 0;
    uint64_t seen = V(vc);
    CHECK("A's third page is not present in B: reading it page-faults", faults == 1 && fault_cr2 == vc, "faults=%d cr2=0x%lx", faults, (unsigned long)fault_cr2);
    CHECK("the fault was a not-present fault", (fault_err & 1) == 0, "error code 0x%lx", (unsigned long)fault_err);
    CHECK("after the handler mapped a page, the read sees zeroes", seen == 0, "read 0x%lx", seen);
    aspace_switch(A);

    uint64_t C = aspace_create();
    uint64_t c2 = aspace_alloc_page(C, vb, 1);     // allocated first, so it gets the lower frame
    uint64_t c1 = aspace_alloc_page(C, va, 1);
    CHECK("the setup gives the two virtual pages non-adjacent frames", c1 && c2 && c1 != c2 + 4096, "c1=0x%lx c2=0x%lx", c1, c2);
    uint8_t data[200];
    for (int i = 0; i < 200; i++) data[i] = (uint8_t)(i + 1);
    int r = aspace_copy_to(C, va + 4000, data, 200);
    CHECK("copy_to returns 0 for a range inside mapped pages", r == 0, "returned %d", r);
    CHECK("the first 96 bytes land at the end of the first page's frame", c1 && memcmp((void *)(uintptr_t)(c1 + 4000), data, 96) == 0, "wrong bytes in the first frame");
    CHECK("the rest continues at the start of the second page's frame", c2 && memcmp((void *)(uintptr_t)c2, data + 96, 104) == 0, "wrong bytes in the second frame");
    CHECK("copy_to does not switch address spaces", cr3_now() == A, "cr3 is now 0x%lx", cr3_now());

    uint8_t big[8192];
    memset(big, 0x77, sizeof big);
    uint8_t first = *(uint8_t *)(uintptr_t)(c1 + 4000);
    r = aspace_copy_to(C, va + 4000, big, 8000);
    CHECK("copy_to fails if a page in the range is missing", r == -1, "returned %d", r);
    CHECK("a failed copy writes nothing at all", *(uint8_t *)(uintptr_t)(c1 + 4000) == first && memcmp((void *)(uintptr_t)c2, data + 96, 104) == 0, "part of the data was written before the failure");
    CHECK("copy_to of zero bytes succeeds anywhere", aspace_copy_to(C, va + 0x500000, big, 0) == 0, "returned non-zero");
    CHECK("copy_to into the kernel half is refused", aspace_copy_to(C, 0x100000, big, 16) == -1, "wrote into kernel space");
    CHECK("copy_to rejects a range that wraps around", aspace_copy_to(C, 0xFFFFFFFFFFFFFFF0ULL, big, 64) == -1 && aspace_copy_to(C, va, big, 0xFFFFFFFFFFFFFF00ULL) == -1, "an overflowing range was accepted");

    size_t base = frames_in_use;
    unsigned n = aspace_destroy(C);
    CHECK("destroying C frees its PML4, PDPT, PD, PT and two data frames", n == 6 && base - frames_in_use == 6, "returned %u, frames in use dropped by %lu", n, (unsigned long)(base - frames_in_use));
    CHECK("destroy never frees a frame twice or one that is not in the pool", bad_frees == 0, "%d bad frees (probably the kernel's tables behind entry 0)", bad_frees);

    n = aspace_destroy(A);
    CHECK("destroying A frees 8 frames: PML4, PDPT, PD, two PTs, three data frames", n == 8, "returned %u", n);
    aspace_switch(B);
    n = aspace_destroy(B);
    CHECK("destroying the current space switches back to the kernel's tables first", cr3_now() == kernel_pml4 && current_as == kernel_pml4, "cr3=0x%lx", cr3_now());
    CHECK("B's tree, including the page the fault handler mapped, is freed too", n == 7, "returned %u (PML4, PDPT, PD, 2 PTs, 2 data = 7)", n);
    CHECK("every frame is back in the pool, with no bad frees", frames_in_use == 0 && bad_frees == 0, "%lu frames still in use, %d bad frees", (unsigned long)frames_in_use, bad_frees);
    CHECK("destroying the kernel's own space does nothing", aspace_destroy(kernel_pml4) == 0 && bad_frees == 0 && marker == 0x600DF00D, "kernel_pml4 was destroyed");
}
