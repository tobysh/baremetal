#include <stdint.h>
#include <stddef.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

extern void *bm_isr_table[48];
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

// ---- Provided ----
typedef struct __attribute__((packed)) {
    uint16_t offset_low, selector;
    uint8_t ist, type_attr;
    uint16_t offset_mid;
    uint32_t offset_high, zero;
} IdtGate;

static IdtGate idt[48];

// An IDT with a gate for every vector from 0 to 47, all using the boot stub's entry code.
static void idt_init(void) {
    for (int i = 0; i < 48; i++) {
        uint64_t a = (uint64_t)(uintptr_t)bm_isr_table[i];
        idt[i] = (IdtGate){(uint16_t)a, 0x08, 0, 0x8E, (uint16_t)(a >> 16), (uint32_t)(a >> 32), 0};
    }
    struct __attribute__((packed)) { uint16_t size; uint64_t base; } p = {sizeof idt - 1, (uint64_t)(uintptr_t)idt};
    __asm__ volatile("lidt %0" : : "m"(p));
}

static inline void sti(void) { __asm__ volatile("sti"); }
static inline void cli(void) { __asm__ volatile("cli"); }

volatile uint64_t ticks;
uint64_t timer_ticks(void) { return ticks; }

// ---- Your code ----

void pic_remap(uint8_t offset1, uint8_t offset2) {
}

void pic_eoi(unsigned irq) {
}

void pic_set_mask(unsigned irq, int masked) {
}

void pit_init(uint32_t hz) {
}

void interrupt_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
}

void timer_init(uint32_t hz) {
}

void sleep_ticks(uint64_t n) {
}
