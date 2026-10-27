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
    GdtEntry e = {0};
    return e;
}

IdtGate idt_gate(uint64_t handler, uint16_t selector, uint8_t type_attr, uint8_t ist) {
    IdtGate g = {0};
    return g;
}

DescriptorPointer descriptor_pointer(const void *table, size_t bytes) {
    DescriptorPointer p = {0, 0};
    return p;
}
