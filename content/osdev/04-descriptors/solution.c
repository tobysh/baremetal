#include <stdint.h>
#include <stddef.h>

typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_high_flags;   // low nibble: limit bits 16-19, high nibble: flags
    uint8_t base_high;
} GdtEntry;

typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
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
