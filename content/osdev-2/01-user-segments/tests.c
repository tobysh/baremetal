static uint8_t kstack[4096] __attribute__((aligned(16)));

static uint16_t read_seg_cs(void) { uint16_t v; __asm__ volatile("mov %%cs, %0" : "=r"(v)); return v; }
static uint16_t read_seg_ss(void) { uint16_t v; __asm__ volatile("mov %%ss, %0" : "=r"(v)); return v; }
static uint16_t read_seg_ds(void) { uint16_t v; __asm__ volatile("mov %%ds, %0" : "=r"(v)); return v; }
static uint16_t read_tr(void) { uint16_t v; __asm__ volatile("str %0" : "=r"(v)); return v; }

void kmain(void) {
    // A descriptor with a recognisable bit pattern in every field.
    TssDescriptor d;
    memset(&d, 0xEE, sizeof d);
    tss_descriptor(&d, 0x1122334455667788ULL, 0x67);
    CHECK("the TSS descriptor is 16 bytes", sizeof(TssDescriptor) == 16, "sizeof is %lu", (unsigned long)sizeof(TssDescriptor));
    CHECK("base bits 0-15 and 16-23 land in bytes 2-4", d.base_low == 0x7788 && d.base_mid == 0x66, "base_low=0x%x base_mid=0x%x", d.base_low, d.base_mid);
    CHECK("base bits 24-31 land in byte 7", d.base_high == 0x55, "base_high=0x%x", d.base_high);
    CHECK("base bits 32-63 land in the upper dword", d.base_upper == 0x11223344, "base_upper=0x%x", d.base_upper);
    CHECK("the limit is 0x67 and the flags nibble is zero", d.limit_low == 0x67 && (d.limit_flags & 0xF0) == 0, "limit_low=0x%x limit_flags=0x%x", d.limit_low, d.limit_flags);
    CHECK("access is 0x89: present, DPL 0, available 64-bit TSS", d.access == 0x89, "access=0x%x", d.access);
    CHECK("the reserved dword is zero", d.reserved == 0, "reserved=0x%x", d.reserved);

    tss_descriptor(&d, 0x1000, 0xABCDE);
    CHECK("limit bits 16-19 go in the low nibble of byte 6", d.limit_low == 0xBCDE && (d.limit_flags & 0x0F) == 0xA, "limit_low=0x%x limit_flags=0x%x", d.limit_low, d.limit_flags);

    Tss t;
    memset(&t, 0xFF, sizeof t);
    tss_init(&t, 0xFFFF800000012340ULL);
    int clean = t.reserved0 == 0 && t.rsp1 == 0 && t.rsp2 == 0 && t.reserved1 == 0 && t.reserved2 == 0 && t.reserved3 == 0;
    for (int i = 0; i < 7; i++) if (t.ist[i]) clean = 0;
    CHECK("tss_init clears every field it does not set", clean, "a field still holds the 0xFF fill");
    CHECK("tss_init sets rsp0", t.rsp0 == 0xFFFF800000012340ULL, "rsp0=0x%lx", t.rsp0);
    CHECK("the I/O bitmap offset is the TSS size, so ring 3 gets no ports", t.iopb_offset == sizeof(Tss), "iopb_offset=%u", t.iopb_offset);

    uint64_t top = (uint64_t)(uintptr_t)(kstack + sizeof kstack);
    gdt_setup(top);

    struct __attribute__((packed)) { uint16_t size; uint64_t base; } g;
    __asm__ volatile("sgdt %0" : "=m"(g));
    const uint8_t *tab = (const uint8_t *)(uintptr_t)g.base;
    CHECK("the loaded GDT is 56 bytes: five 8-byte entries and a 16-byte TSS descriptor", g.size == 55, "GDTR limit is %u (expected 55)", g.size);

    int null_ok = 1;
    for (int i = 0; i < 8; i++) if (tab[i]) null_ok = 0;
    CHECK("entry 0 is the null descriptor", null_ok, "entry 0 is not zero");
    CHECK("0x08 is a ring-0 64-bit code segment (access 0x9A, L bit set)", (tab[8 + 5] & 0xFE) == 0x9A && (tab[8 + 6] & 0xA0) == 0xA0, "access=0x%x byte6=0x%x", tab[13], tab[14]);
    CHECK("0x10 is a ring-0 data segment (access 0x92)", (tab[16 + 5] & 0xFE) == 0x92, "access=0x%x", tab[21]);
    CHECK("0x18 is a ring-3 data segment (access 0xF2)", (tab[24 + 5] & 0xFE) == 0xF2, "access=0x%x", tab[29]);
    CHECK("0x20 is a ring-3 64-bit code segment (access 0xFA, L bit set)", (tab[32 + 5] & 0xFE) == 0xFA && (tab[32 + 6] & 0xA0) == 0xA0, "access=0x%x byte6=0x%x", tab[37], tab[38]);
    CHECK("user data comes right before user code, as sysret requires", (tab[24 + 5] & 0xFE) == 0xF2 && (tab[32 + 5] & 0xFE) == 0xFA, "the order of the user entries is wrong");

    CHECK("the task register holds selector 0x28", read_tr() == 0x28, "str returned 0x%x", read_tr());
    CHECK("the CPU marked the TSS descriptor busy (access 0x8B)", tab[40 + 5] == 0x8B, "access byte is 0x%x (0x89 means ltr never ran on this descriptor)", tab[45]);

    uint64_t base = (uint64_t)tab[42] | ((uint64_t)tab[43] << 8) | ((uint64_t)tab[44] << 16) | ((uint64_t)tab[47] << 24)
                  | ((uint64_t)tab[48] << 32) | ((uint64_t)tab[49] << 40) | ((uint64_t)tab[50] << 48) | ((uint64_t)tab[51] << 56);
    uint32_t limit = tab[40] | (tab[41] << 8) | ((tab[46] & 0x0F) << 16);
    CHECK("the descriptor's limit is sizeof(Tss) - 1", limit == sizeof(Tss) - 1, "limit is %u", limit);
    const Tss *live = (const Tss *)(uintptr_t)base;
    CHECK("the descriptor points at a TSS whose rsp0 is the kernel stack top", base != 0 && live->rsp0 == top, "base=0x%lx rsp0=0x%lx expected 0x%lx", base, base ? live->rsp0 : 0, top);
    CHECK("that TSS has no I/O bitmap", base != 0 && live->iopb_offset >= sizeof(Tss), "iopb_offset=%u", base ? live->iopb_offset : 0);

    CHECK("cs is still the kernel code selector 0x08", read_seg_cs() == 0x08, "cs=0x%x", read_seg_cs());
    CHECK("ss is the kernel data selector 0x10", read_seg_ss() == 0x10, "ss=0x%x", read_seg_ss());
    CHECK("ds is the kernel data selector 0x10", read_seg_ds() == 0x10, "ds=0x%x", read_seg_ds());

    // The CPU only accepts a selector for a present, correctly formed descriptor.
    uint16_t ud = 0x1B, back;
    __asm__ volatile("mov %1, %%ds\n mov %%ds, %0\n mov $0x10, %%ax\n mov %%ax, %%ds" : "=r"(back) : "r"(ud) : "rax");
    CHECK("the CPU accepts 0x1B (user data, RPL 3) in ds", back == 0x1B, "ds read back as 0x%x", back);
}
