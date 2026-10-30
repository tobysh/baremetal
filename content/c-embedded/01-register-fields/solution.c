#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bus {
    uint32_t (*read)(struct bus *b, uint32_t off);
    void     (*write)(struct bus *b, uint32_t off, uint32_t v);
    uint32_t (*now)(struct bus *b);
} bus_t;

static uint32_t width_mask(unsigned width) {
    return width >= 32 ? 0xFFFFFFFFu : (1u << width) - 1u;
}

// Read the field starting at bit `shift`, `width` bits wide.
uint32_t field_get(bus_t *b, uint32_t off, unsigned shift, unsigned width) {
    return (b->read(b, off) >> shift) & width_mask(width);
}

// Change only that field. `val` is truncated to `width` bits.
void field_set(bus_t *b, uint32_t off, unsigned shift, unsigned width, uint32_t val) {
    uint32_t m = width_mask(width);
    uint32_t old = b->read(b, off);
    b->write(b, off, (old & ~(m << shift)) | ((val & m) << shift));
}

/* ---- public example: a plain memory-backed "device" ---- */
typedef struct { bus_t bus; uint32_t regs[4]; } memdev;
static uint32_t mem_read(bus_t *b, uint32_t off) { return ((memdev *)b)->regs[off / 4]; }
static void mem_write(bus_t *b, uint32_t off, uint32_t v) { ((memdev *)b)->regs[off / 4] = v; }
static uint32_t mem_now(bus_t *b) { (void)b; return 0; }

int main(void) {
    memdev d = { { mem_read, mem_write, mem_now }, { 0x00000FF1u, 0, 0, 0 } };
    printf("before: CTRL = 0x%08X\n", d.regs[0]);
    field_set(&d.bus, 0, 1, 2, 2);
    printf("after : CTRL = 0x%08X (expect 0x00000FF5)\n", d.regs[0]);
    printf("DIV   = 0x%X (expect 0xFF)\n", field_get(&d.bus, 0, 4, 8));
    return 0;
}
