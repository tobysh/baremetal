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
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);
    outb(PIC1_CMD, 0x11);          // ICW1: start initialisation, ICW4 follows
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, offset1);      // ICW2: vector offsets
    outb(PIC2_DATA, offset2);
    outb(PIC1_DATA, 4);            // ICW3: slave on IRQ 2
    outb(PIC2_DATA, 2);            //       slave identity 2
    outb(PIC1_DATA, 0x01);         // ICW4: 8086 mode
    outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, mask1);        // initialisation cleared the masks: put them back
    outb(PIC2_DATA, mask2);
}

void pic_eoi(unsigned irq) {
    if (irq >= 8) outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}

void pic_set_mask(unsigned irq, int masked) {
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    uint8_t bit = (uint8_t)(1u << (irq & 7));
    uint8_t value = inb(port);
    outb(port, masked ? (uint8_t)(value | bit) : (uint8_t)(value & ~bit));
}

void pit_init(uint32_t hz) {
    uint32_t divisor = 1193182 / hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)(divisor >> 8));
}

void interrupt_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
    (void)error_code; (void)rip;
    if (vector < 32 || vector > 47) return;
    unsigned irq = (unsigned)(vector - 32);
    if (irq == 7 || irq == 15) {
        uint16_t cmd = irq == 7 ? PIC1_CMD : PIC2_CMD;
        outb(cmd, 0x0B);                       // next read of the command port returns the ISR
        if ((inb(cmd) & 0x80) == 0) {          // not really in service: spurious
            if (irq == 15) outb(PIC1_CMD, 0x20);
            return;
        }
    }
    if (irq == 0) ticks++;
    pic_eoi(irq);
}

void timer_init(uint32_t hz) {
    idt_init();
    pic_remap(32, 40);
    outb(PIC1_DATA, 0xFE);         // only IRQ 0 unmasked
    outb(PIC2_DATA, 0xFF);
    pit_init(hz);
    bm_exception_hook = interrupt_handler;
    sti();
}

void sleep_ticks(uint64_t n) {
    uint64_t start = ticks;
    while (ticks - start < n) __asm__ volatile("hlt");
}
