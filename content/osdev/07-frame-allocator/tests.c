extern uint32_t bm_multiboot_info;

// Builds a Multiboot info block with a memory map of `n` entries, each `stride` bytes apart.
static uint8_t fake_info[64] __attribute__((aligned(8)));
static uint8_t fake_map[256] __attribute__((aligned(8)));

static void put_entry(int index, int stride, uint64_t base, uint64_t len, uint32_t type) {
    uint8_t *e = fake_map + index * stride;
    memset(e, 0, stride);
    *(uint32_t *)e = stride - 4;
    *(uint64_t *)(e + 4) = base;
    *(uint64_t *)(e + 12) = len;
    *(uint32_t *)(e + 20) = type;
}

static void make_info(int n, int stride, uint32_t flags) {
    memset(fake_info, 0, sizeof fake_info);
    *(uint32_t *)fake_info = flags;
    *(uint32_t *)(fake_info + 44) = n * stride;
    *(uint32_t *)(fake_info + 48) = (uint32_t)(uintptr_t)fake_map;
}

static uint8_t bitmap[2048 + 16];
static uint8_t big_bitmap[2048];

void kmain(void) {
    MemRegion r[8];

    put_entry(0, 24, 0, 0x9FC00, 1);
    put_entry(1, 24, 0x9FC00, 0x60400, 2);
    put_entry(2, 24, 0x100000, 0x300000, 1);
    make_info(3, 24, 1u << 6);
    size_t n = mmap_parse(fake_info, r, 8);
    CHECK("mmap_parse returns the number of entries", n == 3, "returned %lu", (unsigned long)n);
    CHECK("mmap_parse reads base, length and type", r[2].base == 0x100000 && r[2].length == 0x300000 && r[2].type == 1 && r[1].type == 2 && r[0].length == 0x9FC00,
          "entry 2: base 0x%lx length 0x%lx type %u", r[2].base, r[2].length, r[2].type);

    n = mmap_parse(fake_info, r, 2);
    CHECK("mmap_parse never stores more than max entries", n == 2, "returned %lu", (unsigned long)n);

    make_info(3, 24, 0);
    CHECK("mmap_parse returns 0 when the memory map flag is clear", mmap_parse(fake_info, r, 8) == 0, "should be 0");

    put_entry(0, 28, 0x1000, 0x1000, 1);
    put_entry(1, 28, 0x5000, 0x2000, 2);
    make_info(2, 28, 1u << 6);
    n = mmap_parse(fake_info, r, 8);
    CHECK("mmap_parse steps by each entry's size field, not a fixed 24", n == 2 && r[1].base == 0x5000 && r[1].type == 2, "returned %lu, second base 0x%lx", (unsigned long)n, r[1].base);

    // ---- bitmap allocator on a synthetic map ----
    memset(bitmap, 0x5A, sizeof bitmap);
    pmm_init(bitmap, 64);
    CHECK("pmm_init marks every frame used", pmm_free_count() == 0 && pmm_alloc() == PMM_NONE, "free count %lu", (unsigned long)pmm_free_count());

    MemRegion usable = {0x1800, 0x3000, MMAP_USABLE};
    pmm_add_region(&usable);
    CHECK("only frames fully inside a region are freed", pmm_free_count() == 2, "free count %lu (expected 2: frames at 0x2000 and 0x3000)", (unsigned long)pmm_free_count());
    pmm_add_region(&usable);
    CHECK("adding a region twice does not inflate the count", pmm_free_count() == 2, "free count %lu", (unsigned long)pmm_free_count());

    MemRegion reserved = {0x10000, 0x10000, 2};
    pmm_add_region(&reserved);
    CHECK("regions that are not usable RAM are ignored", pmm_free_count() == 2, "free count %lu", (unsigned long)pmm_free_count());

    uint64_t a = pmm_alloc(), b = pmm_alloc(), c = pmm_alloc();
    CHECK("pmm_alloc returns the lowest free frame first", a == 0x2000 && b == 0x3000, "got 0x%lx and 0x%lx", a, b);
    CHECK("pmm_alloc returns PMM_NONE when memory is exhausted", c == PMM_NONE && pmm_free_count() == 0, "third alloc returned 0x%lx", c);

    CHECK("pmm_free returns 0 and makes the frame available", pmm_free(0x2000) == 0 && pmm_free_count() == 1 && pmm_alloc() == 0x2000, "free count %lu", (unsigned long)pmm_free_count());
    CHECK("pmm_free rejects a double free", pmm_free(0x3000) == 0 && pmm_free(0x3000) == -1 && pmm_free_count() == 1, "free count %lu", (unsigned long)pmm_free_count());
    CHECK("pmm_free rejects unaligned and out-of-range addresses", pmm_free(0x2001) == -1 && pmm_free(64 * 4096) == -1 && pmm_free(0x1000000) == -1, "should all be -1");

    pmm_init(bitmap, 64);
    MemRegion all = {0, 64 * 4096, MMAP_USABLE};
    pmm_add_region(&all);
    CHECK("a region can cover every frame", pmm_free_count() == 64, "free count %lu", (unsigned long)pmm_free_count());
    pmm_reserve(0x2800, 0x100);
    CHECK("pmm_reserve takes every frame the range touches, even partly", pmm_free_count() == 63 && pmm_free(0x2000) == 0, "free count %lu", (unsigned long)pmm_free_count());
    pmm_reserve(0x2800, 0);
    CHECK("pmm_reserve with length 0 reserves nothing", pmm_free_count() == 64, "free count %lu", (unsigned long)pmm_free_count());
    pmm_reserve(0x3000, 0x2000);
    pmm_reserve(0x3000, 0x2000);
    CHECK("reserving the same range twice is harmless", pmm_free_count() == 62, "free count %lu", (unsigned long)pmm_free_count());
    MemRegion over = {60 * 4096, 100 * 4096, MMAP_USABLE};
    pmm_init(bitmap, 64);
    pmm_add_region(&over);
    CHECK("frames beyond frame_count are ignored", pmm_free_count() == 4, "free count %lu", (unsigned long)pmm_free_count());
    CHECK("the bitmap does not write past its last byte", bitmap[8] == 0x5A, "byte after the bitmap changed to 0x%02x", bitmap[8]);

    // ---- the real machine ----
    MemRegion real[32];
    n = mmap_parse((const void *)(uintptr_t)bm_multiboot_info, real, 32);
    uint64_t usable_total = 0; int has_1m = 0;
    for (size_t i = 0; i < n; i++) if (real[i].type == 1) { usable_total += real[i].length; if (real[i].base == 0x100000) has_1m = 1; }
    CHECK("the real memory map of this 64 MiB machine has usable RAM at 1 MiB", n >= 2 && has_1m, "found %lu entries", (unsigned long)n);
    CHECK("the real usable RAM is between 60 and 64 MiB", usable_total > 60ULL * 1024 * 1024 && usable_total <= 64ULL * 1024 * 1024, "usable total is %lu bytes", usable_total);

    pmm_setup((const void *)(uintptr_t)bm_multiboot_info, big_bitmap, 16384);
    size_t free_frames = pmm_free_count();
    CHECK("pmm_setup leaves most of the 16384 frames free", free_frames > 15500 && free_frames < 16352 - 256, "free count %lu", (unsigned long)free_frames);

    int bad_low = 0, bad_kernel = 0, bad_dup = 0;
    static uint64_t got[200];
    for (int i = 0; i < 200; i++) {
        got[i] = pmm_alloc();
        if (got[i] == PMM_NONE) break;
        if (got[i] < 0x100000) bad_low++;
        if (got[i] < (uint64_t)(uintptr_t)_kernel_end) bad_kernel++;
        for (int j = 0; j < i; j++) if (got[j] == got[i]) bad_dup++;
    }
    CHECK("no frame is handed out below 1 MiB", bad_low == 0, "%d frames came from the first megabyte", bad_low);
    CHECK("no frame is handed out from inside the kernel image", bad_kernel == 0, "%d frames overlap the kernel", bad_kernel);
    CHECK("no frame is handed out twice", bad_dup == 0, "%d duplicates", bad_dup);

    int mem_ok = 1;
    for (int i = 0; i < 200; i++) { volatile uint64_t *p = (volatile uint64_t *)(uintptr_t)got[i]; p[0] = 0x1000 + i; p[511] = 0x2000 + i; }
    for (int i = 0; i < 200; i++) { volatile uint64_t *p = (volatile uint64_t *)(uintptr_t)got[i]; if (p[0] != 0x1000 + (uint64_t)i || p[511] != 0x2000 + (uint64_t)i) mem_ok = 0; }
    CHECK("the frames are real RAM: 200 frames hold their own contents", mem_ok, "a frame overwrote another");
    CHECK("the free count dropped by exactly the frames allocated", pmm_free_count() == free_frames - 200, "free count %lu", (unsigned long)pmm_free_count());
}
