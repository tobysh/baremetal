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
    uint64_t pml4 = frame_alloc();
    if (!pml4) return 0;
    ((uint64_t *)(uintptr_t)pml4)[0] = ((uint64_t *)(uintptr_t)kernel_pml4)[0];   // share the kernel
    return pml4;
}

uint64_t aspace_alloc_page(uint64_t as, uint64_t virt, int writable) {
    if ((virt & 4095) || virt < USER_BASE || virt >= USER_END) return 0;
    uint64_t f = frame_alloc();
    if (!f) return 0;
    if (map_page(as, virt, f, PTE_U | (writable ? PTE_W : 0)) != 0) {
        frame_free(f);
        return 0;
    }
    return f;
}

void aspace_switch(uint64_t as) {
    current_as = as;
    __asm__ volatile("mov %0, %%cr3" : : "r"(as) : "memory");
}

int aspace_copy_to(uint64_t as, uint64_t virt, const void *src, size_t len) {
    if (len == 0) return 0;
    if (virt < USER_BASE || virt >= USER_END || len > USER_END - virt) return -1;
    for (uint64_t a = virt & ~4095ULL; a < virt + len; a += 4096) {
        uint64_t *e = leaf_pte(as, a);
        if (!e || !(*e & PTE_P)) return -1;
    }
    const uint8_t *s = src;
    while (len) {
        uint64_t *e = leaf_pte(as, virt);
        size_t chunk = 4096 - (virt & 4095);
        if (chunk > len) chunk = len;
        memcpy((void *)(uintptr_t)((*e & ADDR_MASK) + (virt & 4095)), s, chunk);
        virt += chunk; s += chunk; len -= chunk;
    }
    return 0;
}

static unsigned free_tree(uint64_t table, int level) {
    unsigned n = 0;
    uint64_t *t = (uint64_t *)(uintptr_t)table;
    for (int i = 0; i < 512; i++) {
        if (!(t[i] & PTE_P)) continue;
        uint64_t target = t[i] & ADDR_MASK;
        if (level == 0) { frame_free(target); n++; }
        else n += free_tree(target, level - 1);
    }
    frame_free(table);
    return n + 1;
}

unsigned aspace_destroy(uint64_t as) {
    if (as == kernel_pml4) return 0;
    if (as == current_as) aspace_switch(kernel_pml4);
    uint64_t *pml4 = (uint64_t *)(uintptr_t)as;
    unsigned n = 0;
    for (int i = 1; i < 512; i++)              // entry 0 is the kernel's, shared with everyone
        if (pml4[i] & PTE_P) n += free_tree(pml4[i] & ADDR_MASK, 2);
    frame_free(as);
    return n + 1;
}
