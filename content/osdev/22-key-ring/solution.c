#include <stdint.h>
#include <stddef.h>

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);

extern void *bm_isr_table[48];
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

#define RING_SIZE 16   // must be a power of two

typedef struct {
    uint8_t data[RING_SIZE];
    volatile uint32_t head;   // total bytes ever pushed
    volatile uint32_t tail;   // total bytes ever popped
} Ring;

// ---- Provided: the interrupt plumbing from the earlier lessons ----
typedef struct __attribute__((packed)) {
    uint16_t offset_low, selector;
    uint8_t ist, type_attr;
    uint16_t offset_mid;
    uint32_t offset_high, zero;
} IdtGate;

static IdtGate idt[48];

static void idt_init(void) {
    for (int i = 0; i < 48; i++) {
        uint64_t a = (uint64_t)(uintptr_t)bm_isr_table[i];
        idt[i] = (IdtGate){(uint16_t)a, 0x08, 0, 0x8E, (uint16_t)(a >> 16), (uint32_t)(a >> 32), 0};
    }
    struct __attribute__((packed)) { uint16_t size; uint64_t base; } p = {sizeof idt - 1, (uint64_t)(uintptr_t)idt};
    __asm__ volatile("lidt %0" : : "m"(p));
}

static void pic_remap(uint8_t off1, uint8_t off2) {
    uint8_t m1 = inb(0x21), m2 = inb(0xA1);
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, off1); outb(0xA1, off2);
    outb(0x21, 4); outb(0xA1, 2);
    outb(0x21, 1); outb(0xA1, 1);
    outb(0x21, m1); outb(0xA1, m2);
}

static void pic_eoi(unsigned irq) {
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

static void pic_set_mask(unsigned irq, int masked) {
    uint16_t port = irq < 8 ? 0x21 : 0xA1;
    uint8_t bit = (uint8_t)(1u << (irq & 7));
    uint8_t v = inb(port);
    outb(port, masked ? (uint8_t)(v | bit) : (uint8_t)(v & ~bit));
}

static void pit_init(uint32_t hz) {
    uint32_t div = 1193182 / hz;
    outb(0x43, 0x34);
    outb(0x40, (uint8_t)(div & 0xFF));
    outb(0x40, (uint8_t)(div >> 8));
}

static uint64_t irq_save(void) {
    uint64_t f;
    __asm__ volatile("pushfq; popq %0; cli" : "=r"(f) : : "memory");
    return f;
}

static void irq_restore(uint64_t f) {
    if (f & (1ull << 9)) __asm__ volatile("sti" : : : "memory");
}

Ring kbd_ring;
volatile uint64_t ticks;

// ---- Your code ----

void ring_init(Ring *r) {
    r->head = 0;
    r->tail = 0;
}

uint32_t ring_count(const Ring *r) {
    return r->head - r->tail;
}

int ring_push(Ring *r, uint8_t b) {
    if (ring_count(r) == RING_SIZE) return 0;
    r->data[r->head & (RING_SIZE - 1)] = b;
    r->head++;
    return 1;
}

int ring_pop(Ring *r, uint8_t *out) {
    if (ring_count(r) == 0) return 0;
    *out = r->data[r->tail & (RING_SIZE - 1)];
    r->tail++;
    return 1;
}

void kbd_irq(void) {
    for (;;) {
        uint8_t st = inb(0x64);
        if (!(st & 1)) break;
        uint8_t b = inb(0x60);
        if (!(st & 0x20)) ring_push(&kbd_ring, b);
    }
}

void interrupt_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
    (void)error_code;
    (void)rip;
    if (vector < 32 || vector > 47) return;
    unsigned irq = (unsigned)(vector - 32);
    if (irq == 7) return;
    if (irq == 15) {
        outb(0x20, 0x20);
        return;
    }
    if (irq == 0) ticks++;
    else if (irq == 1) kbd_irq();
    pic_eoi(irq);
}

void keyboard_init(void) {
    ring_init(&kbd_ring);
    idt_init();
    pic_remap(32, 40);
    for (unsigned i = 0; i < 16; i++) pic_set_mask(i, i != 1);
    bm_exception_hook = interrupt_handler;
    __asm__ volatile("sti" : : : "memory");
}

int kbd_read(void) {
    uint64_t f = irq_save();
    uint8_t b;
    int r = ring_pop(&kbd_ring, &b) ? b : -1;
    irq_restore(f);
    return r;
}

uint8_t kbd_wait(void) {
    for (;;) {
        __asm__ volatile("cli" : : : "memory");
        uint8_t b;
        if (ring_pop(&kbd_ring, &b)) {
            __asm__ volatile("sti" : : : "memory");
            return b;
        }
        __asm__ volatile("sti; hlt" : : : "memory");
    }
}
