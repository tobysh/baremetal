#include <stdint.h>
#include <stddef.h>

void *memset(void *dst, int c, size_t n);
void bm_exit(int code);
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

#define PTE_PRESENT  (1ull << 0)
#define PTE_WRITABLE (1ull << 1)
#define PTE_USER     (1ull << 2)
#define PTE_HUGE     (1ull << 7)
#define PTE_ADDR     0x000FFFFFFFFFF000ull

typedef struct {
    uint64_t start, end;   // [start, end), page aligned
    int writable;
} Region;

// ---- Provided ----

// Physical frames 16 MiB .. 48 MiB, handed out once each. 0 means "out of memory".
static uint64_t next_frame = 0x1000000;
uint64_t frame_alloc(void) {
    if (next_frame >= 0x3000000) return 0;
    uint64_t f = next_frame;
    next_frame += 4096;
    return f;
}

static uint64_t read_cr2(void) {
    uint64_t v;
    __asm__ volatile("mov %%cr2, %0" : "=r"(v));
    return v;
}

static void panic(const char *msg) {
    (void)msg;
    bm_exit(2);
}

// Maps the 4 KiB page `virt` to the frame `phys` in the live page tables (RAM is identity-mapped,
// so a physical address can be used as a pointer). Missing tables are allocated and zeroed.
// Returns 0, or -1 if memory ran out or a huge page is in the way.
static int map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *table = (uint64_t *)(uintptr_t)(cr3 & PTE_ADDR);
    for (int level = 4; level > 1; level--) {
        unsigned idx = (unsigned)((virt >> (12 + 9 * (level - 1))) & 511);
        if (!(table[idx] & PTE_PRESENT)) {
            uint64_t f = frame_alloc();
            if (!f) return -1;
            memset((void *)(uintptr_t)f, 0, 4096);
            table[idx] = f | PTE_PRESENT | PTE_WRITABLE;
        }
        if (table[idx] & PTE_HUGE) return -1;
        table = (uint64_t *)(uintptr_t)(table[idx] & PTE_ADDR);
    }
    table[(virt >> 12) & 511] = (phys & PTE_ADDR) | flags | PTE_PRESENT;
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return 0;
}

uint64_t pages_faulted;

// ---- Your code ----

int vm_region_add(uint64_t start, uint64_t len, int writable) {
    return -1;
}

int pf_handle(uint64_t addr, uint64_t err) {
    return 0;
}

void pf_exception_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
}

void vm_init(void) {
}

// Optional: press Run to touch a lazy page.
void kmain(void) {
    vm_init();
    vm_region_add(0x80000000, 4 * 4096, 1);
    volatile uint64_t *p = (volatile uint64_t *)0x80000000;
    *p = 42;
}
