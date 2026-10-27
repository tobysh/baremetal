static uint64_t as_u64(GdtEntry e) { uint64_t v; memcpy(&v, &e, 8); return v; }

void kmain(void) {
    CHECK("GdtEntry is 8 bytes and IdtGate is 16", sizeof(GdtEntry) == 8 && sizeof(IdtGate) == 16 && sizeof(DescriptorPointer) == 10,
          "sizes are %lu, %lu, %lu", (unsigned long)sizeof(GdtEntry), (unsigned long)sizeof(IdtGate), (unsigned long)sizeof(DescriptorPointer));

    uint64_t v = as_u64(gdt_entry(0, 0, 0, 0));
    CHECK("the null descriptor is all zeros", v == 0, "got 0x%016lx", v);

    v = as_u64(gdt_entry(0, 0xFFFFF, 0x9A, 0xC));
    CHECK("flat 32-bit code segment is 0x00CF9A000000FFFF", v == 0x00CF9A000000FFFFULL, "got 0x%016lx", v);

    v = as_u64(gdt_entry(0, 0xFFFFF, 0x92, 0xC));
    CHECK("flat 32-bit data segment is 0x00CF92000000FFFF", v == 0x00CF92000000FFFFULL, "got 0x%016lx", v);

    v = as_u64(gdt_entry(0, 0, 0x9A, 0xA));
    CHECK("64-bit code segment (L flag) is 0x00A09A0000000000", v == 0x00A09A0000000000ULL, "got 0x%016lx", v);

    v = as_u64(gdt_entry(0, 0xFFFFF, 0xFA, 0xA));
    CHECK("ring-3 access byte 0xFA is stored in byte 5", ((v >> 40) & 0xFF) == 0xFA, "byte 5 is 0x%02lx", (v >> 40) & 0xFF);

    v = as_u64(gdt_entry(0x12345678, 0xABCDE, 0x9A, 0xC));
    CHECK("base and limit are split across the entry", v == 0x12CA9A345678BCDEULL, "got 0x%016lx (expected 0x12CA9A345678BCDE)", v);

    v = as_u64(gdt_entry(0xFFFFFFFF, 0xFFFFFFFF, 0x92, 0xFF));
    CHECK("oversized limit and flags are masked, not smeared", v == 0xFFFF92FFFFFFFFFFULL, "got 0x%016lx", v);

    IdtGate g = idt_gate(0xFFFF800012345678ULL, 0x08, 0x8E, 1);
    CHECK("idt_gate splits the handler into three pieces",
          g.offset_low == 0x5678 && g.offset_mid == 0x1234 && g.offset_high == 0xFFFF8000,
          "low 0x%04x mid 0x%04x high 0x%08x", g.offset_low, g.offset_mid, g.offset_high);
    CHECK("idt_gate keeps selector, type and IST", g.selector == 0x08 && g.type_attr == 0x8E && g.ist == 1,
          "selector 0x%04x type 0x%02x ist %u", g.selector, g.type_attr, g.ist);
    CHECK("idt_gate zeros the reserved dword", g.zero == 0, "reserved is 0x%08x", g.zero);

    g = idt_gate(0x100000, 0x18, 0xEE, 0xFF);
    CHECK("the IST index is limited to 3 bits", g.ist == 7, "ist is %u", g.ist);

    uint8_t raw[16]; memcpy(raw, &g, 16);
    CHECK("an idt_gate really is laid out low address first",
          raw[0] == 0x00 && raw[1] == 0x00 && raw[2] == 0x18 && raw[5] == 0xEE && raw[6] == 0x10 && raw[7] == 0x00,
          "bytes: %02x %02x %02x ... %02x %02x %02x", raw[0], raw[1], raw[2], raw[5], raw[6], raw[7]);

    static GdtEntry table[3];
    DescriptorPointer p = descriptor_pointer(table, sizeof table);
    CHECK("descriptor_pointer stores size minus one", p.size == 23, "size is %u", p.size);
    CHECK("descriptor_pointer stores the table address", p.base == (uint64_t)(uintptr_t)table, "base is 0x%lx", p.base);
}
