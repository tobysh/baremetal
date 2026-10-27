#include <stdint.h>
#include <stddef.h>

#define FRAME_SIZE  4096
#define MMAP_USABLE 1
#define PMM_NONE    ((uint64_t)-1)

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;   // 1 = usable RAM
} MemRegion;

extern char _kernel_end[];          // first address after the kernel image (from the linker script)
extern uint32_t bm_multiboot_info;  // address of the Multiboot info structure (from the boot stub)

size_t mmap_parse(const void *mb_info, MemRegion *out, size_t max) {
    const uint8_t *info = mb_info;
    uint32_t flags = *(const uint32_t *)info;
    if (!(flags & (1u << 6))) return 0;
    uint32_t length = *(const uint32_t *)(info + 44);
    const uint8_t *entry = (const uint8_t *)(uintptr_t) * (const uint32_t *)(info + 48);
    size_t count = 0;
    for (uint32_t off = 0; off < length && count < max;) {
        const uint8_t *e = entry + off;
        uint32_t size = *(const uint32_t *)e;
        out[count].base = *(const uint64_t *)(e + 4);
        out[count].length = *(const uint64_t *)(e + 12);
        out[count].type = *(const uint32_t *)(e + 20);
        count++;
        off += size + 4;
    }
    return count;
}

static uint8_t *bm;
static size_t nframes, nfree;

static int is_used(size_t i) { return (bm[i / 8] >> (i % 8)) & 1; }

static void mark_used(size_t i) {
    if (!is_used(i)) { bm[i / 8] |= (uint8_t)(1u << (i % 8)); nfree--; }
}

static void mark_free(size_t i) {
    if (is_used(i)) { bm[i / 8] &= (uint8_t)~(1u << (i % 8)); nfree++; }
}

void pmm_init(uint8_t *bitmap, size_t frame_count) {
    bm = bitmap;
    nframes = frame_count;
    nfree = 0;
    for (size_t i = 0; i < (frame_count + 7) / 8; i++) bm[i] = 0xFF;
}

void pmm_add_region(const MemRegion *r) {
    if (r->type != MMAP_USABLE) return;
    uint64_t first = (r->base + FRAME_SIZE - 1) / FRAME_SIZE;
    uint64_t end = (r->base + r->length) / FRAME_SIZE;
    if (end > nframes) end = nframes;
    for (uint64_t f = first; f < end; f++) mark_free((size_t)f);
}

void pmm_reserve(uint64_t base, uint64_t length) {
    if (length == 0) return;
    uint64_t first = base / FRAME_SIZE;
    uint64_t end = (base + length + FRAME_SIZE - 1) / FRAME_SIZE;
    if (end > nframes) end = nframes;
    for (uint64_t f = first; f < end; f++) mark_used((size_t)f);
}

uint64_t pmm_alloc(void) {
    for (size_t i = 0; i < nframes; i++) {
        if (!is_used(i)) {
            mark_used(i);
            return (uint64_t)i * FRAME_SIZE;
        }
    }
    return PMM_NONE;
}

int pmm_free(uint64_t addr) {
    if (addr % FRAME_SIZE != 0 || addr / FRAME_SIZE >= nframes) return -1;
    size_t i = (size_t)(addr / FRAME_SIZE);
    if (!is_used(i)) return -1;
    mark_free(i);
    return 0;
}

size_t pmm_free_count(void) {
    return nfree;
}

void pmm_setup(const void *mb_info, uint8_t *bitmap, size_t frame_count) {
    MemRegion regions[32];
    size_t n = mmap_parse(mb_info, regions, 32);
    pmm_init(bitmap, frame_count);
    for (size_t i = 0; i < n; i++) pmm_add_region(&regions[i]);
    pmm_reserve(0, 0x100000);
    pmm_reserve(0x100000, (uint64_t)(uintptr_t)_kernel_end - 0x100000);
}
