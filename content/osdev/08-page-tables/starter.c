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

unsigned pt_index(uint64_t virt, int level) {
    return 0;
}

uint64_t translate(uint64_t cr3, uint64_t virt, uint64_t *perms) {
    return TRANSLATE_FAULT;
}
