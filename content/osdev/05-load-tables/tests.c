typedef struct __attribute__((packed)) { uint16_t limit; uint64_t base; } Dtr;

static uint16_t seg_cs(void) { uint16_t v; __asm__ volatile("mov %%cs, %0" : "=r"(v)); return v; }
static uint16_t seg_ss(void) { uint16_t v; __asm__ volatile("mov %%ss, %0" : "=r"(v)); return v; }
static uint16_t seg_ds(void) { uint16_t v; __asm__ volatile("mov %%ds, %0" : "=r"(v)); return v; }

// Decoy tables so the tests can tell that *your* install functions really ran.
static uint64_t decoy_gdt[5] = {0, 0x00AF9A000000FFFFULL, 0x00CF92000000FFFFULL, 0, 0};

void kmain(void) {
    Dtr g = {sizeof decoy_gdt - 1, (uint64_t)(uintptr_t)decoy_gdt};
    __asm__ volatile("lgdt %0" : : "m"(g));
    Dtr z = {0, 0};
    __asm__ volatile("lidt %0" : : "m"(z));

    gdt_install();
    Dtr d; __asm__ volatile("sgdt %0" : "=m"(d));
    CHECK("gdt_install loads a GDT with 3 entries (limit 23)", d.limit == 23, "GDT limit is %u", d.limit);
    CHECK("gdt_install replaces the previous GDT", d.base != (uint64_t)(uintptr_t)decoy_gdt, "the GDT is still the old one");

    const uint8_t *e = (const uint8_t *)(uintptr_t)d.base;
    int null_ok = 1;
    for (int i = 0; i < 8; i++) if (e[i]) null_ok = 0;
    CHECK("GDT entry 0 is the null descriptor", null_ok, "entry 0 is not all zeros");
    CHECK("GDT entry 1 is a present ring-0 code segment (access 0x9A)", (e[8 + 5] & 0xFE) == 0x9A, "access byte is 0x%02x", e[8 + 5]);
    CHECK("the code segment has the 64-bit L flag and not the D flag", ((e[8 + 6] >> 5) & 1) == 1 && ((e[8 + 6] >> 6) & 1) == 0,
          "flags nibble is 0x%x (want 0xA)", e[8 + 6] >> 4);
    CHECK("GDT entry 2 is a ring-0 data segment (access 0x92)", (e[16 + 5] & 0xFE) == 0x92, "access byte is 0x%02x", e[16 + 5]);
    CHECK("cs holds the kernel code selector 0x08", seg_cs() == 0x08, "cs is 0x%x", seg_cs());
    CHECK("ds and ss hold the kernel data selector 0x10", seg_ds() == 0x10 && seg_ss() == 0x10, "ds is 0x%x, ss is 0x%x", seg_ds(), seg_ss());

    idt_install();
    Dtr i; __asm__ volatile("sidt %0" : "=m"(i));
    CHECK("idt_install loads an IDT with 32 gates (limit 511)", i.limit == 511, "IDT limit is %u", i.limit);
    const uint8_t *gate = (const uint8_t *)(uintptr_t)i.base + 3 * 16;
    uint64_t addr = (uint64_t)(gate[0] | gate[1] << 8) | ((uint64_t)(gate[6] | gate[7] << 8) << 16)
                  | ((uint64_t)(gate[8] | gate[9] << 8 | gate[10] << 16 | (uint32_t)gate[11] << 24) << 32);
    CHECK("gate 3 points at the entry stub for vector 3", addr == (uint64_t)(uintptr_t)bm_isr_table[3], "gate 3 handler is 0x%lx, want 0x%lx", addr, (uint64_t)(uintptr_t)bm_isr_table[3]);
    CHECK("gate 3 uses selector 0x08 and type 0x8E", gate[2] == 0x08 && gate[3] == 0x00 && gate[5] == 0x8E, "selector 0x%02x%02x, type 0x%02x", gate[3], gate[2], gate[5]);
    const uint8_t *g31 = (const uint8_t *)(uintptr_t)i.base + 31 * 16;
    CHECK("gate 31 is present as well", g31[5] == 0x8E, "type byte of gate 31 is 0x%02x", g31[5]);

    exceptions_init();
    __asm__ volatile("int3");
    CHECK("int3 reaches exception_handler with vector 3", exception_count == 1 && last_vector == 3, "count %lu, last vector %lu", exception_count, last_vector);
    CHECK("the handler sees an instruction address inside the kernel", last_rip >= 0x100000 && last_rip < 0x4000000, "rip is 0x%lx", last_rip);

    __asm__ volatile("int $20");
    CHECK("execution continues after the handler and a second vector is seen", exception_count == 2 && last_vector == 20, "count %lu, last vector %lu", exception_count, last_vector);
    __asm__ volatile("int $31");
    CHECK("vector 31 is wired up too", exception_count == 3 && last_vector == 31, "count %lu, last vector %lu", exception_count, last_vector);

    uint64_t rbx_before = 0x1122334455667788ULL, rbx_after;
    __asm__ volatile("mov %1, %%rbx\n int3\n mov %%rbx, %0" : "=r"(rbx_after) : "r"(rbx_before) : "rbx");
    CHECK("registers survive an exception", rbx_after == rbx_before, "rbx became 0x%lx", rbx_after);
}
