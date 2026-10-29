#include <stdint.h>
#include <stddef.h>

void *memset(void *d, int c, size_t n);

typedef struct __attribute__((packed)) {
    uint16_t limit_low, base_low;
    uint8_t base_mid, access, limit_flags, base_high;
} GdtEntry;

typedef struct __attribute__((packed)) {
    uint16_t limit_low, base_low;
    uint8_t base_mid, access, limit_flags, base_high;
    uint32_t base_upper, reserved;
} TssDescriptor;

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0, rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} Tss;

typedef struct __attribute__((packed)) {
    GdtEntry entry[5];          // null, kernel code, kernel data, user data, user code
    TssDescriptor tss;          // selector 0x28, takes two slots
} Gdt;

typedef struct __attribute__((packed)) { uint16_t size; uint64_t base; } DescriptorPointer;

static Gdt gdt;
static Tss tss;

// ---- Provided ----

// As in Part 1: pack base, 20-bit limit, access byte and 4-bit flags into one 8-byte entry.
static GdtEntry gdt_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    GdtEntry e;
    e.limit_low = (uint16_t)(limit & 0xFFFF);
    e.base_low = (uint16_t)(base & 0xFFFF);
    e.base_mid = (uint8_t)((base >> 16) & 0xFF);
    e.access = access;
    e.limit_flags = (uint8_t)(((limit >> 16) & 0x0F) | (flags << 4));
    e.base_high = (uint8_t)((base >> 24) & 0xFF);
    return e;
}

// Loads the GDT and reloads every segment register: ds/es/ss with 0x10, cs with 0x08 through a far return.
static void gdt_flush(const DescriptorPointer *p) {
    __asm__ volatile(
        "lgdt (%0)\n"
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        : : "r"(p) : "rax", "memory");
}

static void load_task_register(uint16_t selector) {
    __asm__ volatile("ltr %0" : : "r"(selector));
}

// ---- Your code ----

void tss_descriptor(TssDescriptor *out, uint64_t base, uint32_t limit) {
}

void tss_init(Tss *t, uint64_t kernel_stack_top) {
}

void gdt_setup(uint64_t kernel_stack_top) {
}
