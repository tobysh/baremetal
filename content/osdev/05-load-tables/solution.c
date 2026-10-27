#include <stdint.h>
#include <stddef.h>

// ---- Provided by the boot stub ----
extern void *bm_isr_table[48];  // address of the assembly entry stub for each vector
extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

// ---- From the last lesson ----
typedef struct __attribute__((packed)) {
    uint16_t limit_low, base_low;
    uint8_t base_mid, access, limit_high_flags, base_high;
} GdtEntry;

typedef struct __attribute__((packed)) {
    uint16_t offset_low, selector;
    uint8_t ist, type_attr;
    uint16_t offset_mid;
    uint32_t offset_high, zero;
} IdtGate;

typedef struct __attribute__((packed)) {
    uint16_t size;
    uint64_t base;
} DescriptorPointer;

GdtEntry gdt_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    GdtEntry e;
    e.limit_low = (uint16_t)(limit & 0xFFFF);
    e.base_low = (uint16_t)(base & 0xFFFF);
    e.base_mid = (uint8_t)((base >> 16) & 0xFF);
    e.access = access;
    e.limit_high_flags = (uint8_t)(((flags & 0x0F) << 4) | ((limit >> 16) & 0x0F));
    e.base_high = (uint8_t)((base >> 24) & 0xFF);
    return e;
}

IdtGate idt_gate(uint64_t handler, uint16_t selector, uint8_t type_attr, uint8_t ist) {
    IdtGate g;
    g.offset_low = (uint16_t)(handler & 0xFFFF);
    g.selector = selector;
    g.ist = ist & 7;
    g.type_attr = type_attr;
    g.offset_mid = (uint16_t)((handler >> 16) & 0xFFFF);
    g.offset_high = (uint32_t)(handler >> 32);
    g.zero = 0;
    return g;
}

DescriptorPointer descriptor_pointer(const void *table, size_t bytes) {
    DescriptorPointer p;
    p.size = (uint16_t)(bytes - 1);
    p.base = (uint64_t)(uintptr_t)table;
    return p;
}

// ---- Tables and state ----
static GdtEntry gdt[3];
static IdtGate idt[256];

volatile uint64_t last_vector = 0xFFFF;
volatile uint64_t last_error;
volatile uint64_t last_rip;
volatile uint64_t exception_count;

// ---- Assembly helpers: read these, they are short ----

// Load the GDT, then reload every segment register. `cs` cannot be written with mov, so we
// push the new selector and a return address and "return" far into the same place.
static void load_gdt(const DescriptorPointer *p) {
    __asm__ volatile("lgdt %0" : : "m"(*p));
    __asm__ volatile(
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        : : : "rax", "memory");
}

static void load_idt(const DescriptorPointer *p) {
    __asm__ volatile("lidt %0" : : "m"(*p));
}

// ---- Your code ----

void gdt_install(void) {
    gdt[0] = gdt_entry(0, 0, 0, 0);
    gdt[1] = gdt_entry(0, 0xFFFFF, 0x9A, 0xA);   // 64-bit code: L flag set, D flag clear
    gdt[2] = gdt_entry(0, 0xFFFFF, 0x92, 0xC);
    DescriptorPointer p = descriptor_pointer(gdt, sizeof gdt);
    load_gdt(&p);
}

void idt_install(void) {
    for (int i = 0; i < 32; i++) idt[i] = idt_gate((uint64_t)(uintptr_t)bm_isr_table[i], 0x08, 0x8E, 0);
    DescriptorPointer p = descriptor_pointer(idt, 32 * sizeof(IdtGate));
    load_idt(&p);
}

void exception_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
    last_vector = vector;
    last_error = error_code;
    last_rip = rip;
    exception_count++;
}

void exceptions_init(void) {
    gdt_install();
    idt_install();
    bm_exception_hook = exception_handler;
}
