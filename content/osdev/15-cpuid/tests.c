static uint32_t raw_eax, raw_ebx, raw_ecx, raw_edx;
static void ref_cpuid(uint32_t leaf, uint32_t sub) {
    __asm__ volatile("cpuid" : "=a"(raw_eax), "=b"(raw_ebx), "=c"(raw_ecx), "=d"(raw_edx) : "a"(leaf), "c"(sub));
}

void kmain(void) {
    uint32_t r[4] = {0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA};
    cpuid_raw(1, 0, r);
    ref_cpuid(1, 0);
    CHECK("cpuid_raw stores eax, ebx, ecx, edx in order",
          r[0] == raw_eax && r[2] == raw_ecx && r[3] == raw_edx && (r[1] >> 24) == (raw_ebx >> 24),
          "leaf 1 gave %08x %08x %08x %08x, expected %08x %08x %08x %08x", r[0], r[1], r[2], r[3], raw_eax, raw_ebx, raw_ecx, raw_edx);

    cpuid_raw(0, 0, r);
    ref_cpuid(0, 0);
    CHECK("cpuid_raw leaf 0 returns the vendor registers",
          r[0] == raw_eax && r[1] == raw_ebx && r[2] == raw_ecx && r[3] == raw_edx,
          "leaf 0 gave %08x %08x %08x %08x, expected %08x %08x %08x %08x", r[0], r[1], r[2], r[3], raw_eax, raw_ebx, raw_ecx, raw_edx);

    char v[16];
    memset(v, 'Z', sizeof v);
    cpu_vendor(v);
    char want[13];
    memcpy(want, &raw_ebx, 4); memcpy(want + 4, &raw_edx, 4); memcpy(want + 8, &raw_ecx, 4); want[12] = 0;
    CHECK("cpu_vendor spells the vendor in ebx, edx, ecx order", memcmp(v, want, 13) == 0, "got \"%.12s\", expected \"%.12s\"", v, want);
    CHECK("cpu_vendor ends the string with a NUL and writes nothing after it", v[12] == 0 && v[13] == 'Z', "byte 12 is %d, byte 13 is %d", v[12], v[13]);
    CHECK("the vendor string is a real vendor",
          memcmp(v, "GenuineIntel", 12) == 0 || memcmp(v, "AuthenticAMD", 12) == 0 || memcmp(v, "TCGTCGTCGTCG", 12) == 0 || memcmp(v, "KVMKVMKVM\0\0\0", 12) == 0,
          "vendor is \"%.12s\"", v);

    CHECK("cpu_max_leaf matches leaf 0", cpu_max_leaf() == raw_eax && cpu_max_leaf() >= 1, "cpu_max_leaf() is %u, expected %u", cpu_max_leaf(), raw_eax);

    ref_cpuid(1, 0);
    uint32_t edx1 = raw_edx, ecx1 = raw_ecx;
    CHECK("cpu_feature(CPU_EDX, 0): the FPU is there", cpu_feature(CPU_EDX, 0) == 1, "returned %d", cpu_feature(CPU_EDX, 0));
    CHECK("cpu_feature(CPU_EDX, 6): PAE is there", cpu_feature(CPU_EDX, 6) == 1, "returned %d", cpu_feature(CPU_EDX, 6));
    int agree = 1;
    for (int b = 0; b < 32; b++) {
        if (cpu_feature(CPU_EDX, b) != (int)((edx1 >> b) & 1)) agree = 0;
        if (cpu_feature(CPU_ECX, b) != (int)((ecx1 >> b) & 1)) agree = 0;
    }
    CHECK("cpu_feature agrees with cpuid for all 64 bits", agree, "some bit differs from the real leaf 1 (edx %08x, ecx %08x)", edx1, ecx1);
    CHECK("cpu_feature rejects a bad register or bit", cpu_feature(2, 0) == 0 && cpu_feature(CPU_EDX, 32) == 0 && cpu_feature(CPU_EDX, -1) == 0, "an invalid argument returned 1");

    CHECK("cpu_has_long_mode reports 64-bit support", cpu_has_long_mode() == 1, "returned %d", cpu_has_long_mode());

    uint64_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    CHECK("read_cr0 sees protected mode and paging", read_cr0() == cr0 && (read_cr0() & 0x80000001u) == 0x80000001u, "CR0 is 0x%lx, expected 0x%lx", read_cr0(), cr0);
    CHECK("read_cr4 sees PAE", read_cr4() == cr4 && (read_cr4() & 0x20) != 0, "CR4 is 0x%lx, expected 0x%lx", read_cr4(), cr4);

    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000080));
    uint64_t efer = ((uint64_t)hi << 32) | lo;
    CHECK("read_msr combines edx:eax into one 64-bit value", read_msr(0xC0000080) == efer, "EFER read as 0x%lx, expected 0x%lx", read_msr(0xC0000080), efer);
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x1B));
    CHECK("read_msr keeps the high half of a wide MSR", read_msr(0x1B) == (((uint64_t)hi << 32) | lo), "IA32_APIC_BASE read as 0x%lx", read_msr(0x1B));
    CHECK("long_mode_active reports EFER.LMA", long_mode_active() == 1, "returned %d", long_mode_active());
}
