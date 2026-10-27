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
    return 0;
}

void pmm_init(uint8_t *bitmap, size_t frame_count) {
}

void pmm_add_region(const MemRegion *r) {
}

void pmm_reserve(uint64_t base, uint64_t length) {
}

uint64_t pmm_alloc(void) {
    return PMM_NONE;
}

int pmm_free(uint64_t addr) {
    return -1;
}

size_t pmm_free_count(void) {
    return 0;
}

void pmm_setup(const void *mb_info, uint8_t *bitmap, size_t frame_count) {
}
