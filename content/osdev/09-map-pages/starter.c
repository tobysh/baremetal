#include <stdint.h>
#include <stddef.h>

#define PTE_PRESENT  (1ULL << 0)
#define PTE_WRITABLE (1ULL << 1)
#define PTE_USER     (1ULL << 2)
#define PTE_HUGE     (1ULL << 7)
#define PTE_NX       (1ULL << 63)
#define PTE_ADDR_MASK 0x000FFFFFFFFFF000ULL
#define TRANSLATE_FAULT ((uint64_t)-1)

static inline uint64_t read_cr3(void) {
    uint64_t v;
    __asm__ volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}

// The kernel is identity-mapped, so a physical address doubles as a pointer to the same bytes.
static inline uint64_t *table_at(uint64_t phys) { return (uint64_t *)(uintptr_t)phys; }

static inline void tlb_invalidate(uint64_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// A small pool of zeroed frames for new page tables.
#define POOL_FRAMES 48
static uint8_t pool[POOL_FRAMES][4096] __attribute__((aligned(4096)));
size_t frames_used;

uint64_t alloc_frame(void) {
    if (frames_used == POOL_FRAMES) return 0;
    uint8_t *f = pool[frames_used++];
    for (int i = 0; i < 4096; i++) f[i] = 0;
    return (uint64_t)(uintptr_t)f;
}


unsigned pt_index(uint64_t virt, int level) {
    return (unsigned)((virt >> (12 + 9 * (level - 1))) & 0x1FF);
}

uint64_t translate(uint64_t cr3, uint64_t virt, uint64_t *perms) {
    int64_t top = (int64_t)virt >> 47;
    if (top != 0 && top != -1) return TRANSLATE_FAULT;

    int writable = 1, user = 1, nx = 0;
    uint64_t table = cr3 & PTE_ADDR_MASK;
    uint64_t result = TRANSLATE_FAULT;

    for (int level = 4; level >= 1; level--) {
        uint64_t entry = table_at(table)[pt_index(virt, level)];
        if (!(entry & PTE_PRESENT)) return TRANSLATE_FAULT;
        writable &= (entry & PTE_WRITABLE) != 0;
        user &= (entry & PTE_USER) != 0;
        nx |= (entry & PTE_NX) != 0;

        if (level == 3 && (entry & PTE_HUGE)) {
            result = (entry & 0x000FFFFFC0000000ULL) | (virt & 0x3FFFFFFFULL);
            break;
        }
        if (level == 2 && (entry & PTE_HUGE)) {
            result = (entry & 0x000FFFFFFFE00000ULL) | (virt & 0x1FFFFFULL);
            break;
        }
        if (level == 1) {
            result = (entry & PTE_ADDR_MASK) | (virt & 0xFFF);
            break;
        }
        table = entry & PTE_ADDR_MASK;
    }

    if (perms) *perms = (writable ? PTE_WRITABLE : 0) | (user ? PTE_USER : 0) | (nx ? PTE_NX : 0);
    return result;
}

// ---- Your code ----

int map_page(uint64_t cr3, uint64_t virt, uint64_t phys, uint64_t flags) {
    return -1;
}

int unmap_page(uint64_t cr3, uint64_t virt) {
    return -1;
}
