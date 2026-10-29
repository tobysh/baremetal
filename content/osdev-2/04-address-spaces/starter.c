#include <stdint.h>
#include <stddef.h>

void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);

#define PTE_P 1ULL
#define PTE_W 2ULL
#define PTE_U 4ULL
#define ADDR_MASK 0x000FFFFFFFFFF000ULL
#define USER_BASE 0x0000008000000000ULL     // PML4 entry 1: 512 GiB
#define USER_END  0x0000800000000000ULL     // the end of the lower canonical half

// ---- Provided ----

#define POOL_FRAMES 64
static uint8_t pool[POOL_FRAMES][4096] __attribute__((aligned(4096)));
static uint8_t pool_used[POOL_FRAMES];
size_t frames_in_use;
int bad_frees;                              // frees of addresses that are not live pool frames

// A zeroed 4 KiB frame (its physical address equals its pointer), or 0 when the pool is empty.
uint64_t frame_alloc(void) {
    for (int i = 0; i < POOL_FRAMES; i++) {
        if (pool_used[i]) continue;
        pool_used[i] = 1;
        frames_in_use++;
        memset(pool[i], 0, 4096);
        return (uint64_t)(uintptr_t)pool[i];
    }
    return 0;
}

void frame_free(uint64_t addr) {
    uint64_t base = (uint64_t)(uintptr_t)pool;
    if (addr < base || addr >= base + sizeof pool || (addr & 4095) || !pool_used[(addr - base) / 4096]) { bad_frees++; return; }
    pool_used[(addr - base) / 4096] = 0;
    frames_in_use--;
}

uint64_t kernel_pml4;                       // the boot page tables
uint64_t current_as;                        // the PML4 now loaded in cr3

void aspace_boot(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    kernel_pml4 = current_as = cr3 & ADDR_MASK;
}

// Maps one 4 KiB page, creating missing tables (present, writable, user). flags: PTE_W and PTE_U only.
// Returns 0, or -1 if an address is unaligned, a huge page is in the way, the page is already mapped or no frame is left.
int map_page(uint64_t pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    if ((virt | phys) & 4095) return -1;
    uint64_t *table = (uint64_t *)(uintptr_t)pml4;
    for (int level = 3; level > 0; level--) {
        uint64_t *e = &table[(virt >> (12 + 9 * level)) & 511];
        if (!(*e & PTE_P)) {
            uint64_t f = frame_alloc();
            if (!f) return -1;
            *e = f | PTE_P | PTE_W | PTE_U;
        } else if (*e & 0x80) return -1;
        table = (uint64_t *)(uintptr_t)(*e & ADDR_MASK);
    }
    uint64_t *e = &table[(virt >> 12) & 511];
    if (*e & PTE_P) return -1;
    *e = phys | PTE_P | (flags & (PTE_W | PTE_U));
    return 0;
}

// The final (PT) entry for `virt` in the tree under `pml4`, or NULL if a higher level is missing.
uint64_t *leaf_pte(uint64_t pml4, uint64_t virt) {
    uint64_t *table = (uint64_t *)(uintptr_t)pml4;
    for (int level = 3; level > 0; level--) {
        uint64_t e = table[(virt >> (12 + 9 * level)) & 511];
        if (!(e & PTE_P) || (e & 0x80)) return NULL;
        table = (uint64_t *)(uintptr_t)(e & ADDR_MASK);
    }
    return &table[(virt >> 12) & 511];
}

// ---- Your code ----

uint64_t aspace_create(void) {
    return 0;
}

uint64_t aspace_alloc_page(uint64_t as, uint64_t virt, int writable) {
    return 0;
}

void aspace_switch(uint64_t as) {
}

int aspace_copy_to(uint64_t as, uint64_t virt, const void *src, size_t len) {
    return -1;
}

unsigned aspace_destroy(uint64_t as) {
    return 0;
}
