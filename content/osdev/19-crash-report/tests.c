static int eq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int starts(const char *s, const char *p) { while (*p) if (*s++ != *p++) return 0; return 1; }

static char buf[200];

#define PF(err, want) do { \
    memset(buf, '#', sizeof buf); \
    describe_page_fault(err, buf, sizeof buf); \
    CHECK("describe_page_fault(" #err ")", eq(buf, want), "got \"%s\", expected \"%s\"", buf, want); } while (0)

void kmain(void) {
    CHECK("exception_name: the first vectors", eq(exception_name(0), "Divide Error") && eq(exception_name(3), "Breakpoint") && eq(exception_name(6), "Invalid Opcode"),
          "got \"%s\", \"%s\", \"%s\"", exception_name(0), exception_name(3), exception_name(6));
    CHECK("exception_name: faults with error codes", eq(exception_name(8), "Double Fault") && eq(exception_name(13), "General Protection Fault") && eq(exception_name(14), "Page Fault"),
          "got \"%s\", \"%s\", \"%s\"", exception_name(8), exception_name(13), exception_name(14));
    CHECK("exception_name: the later vectors", eq(exception_name(16), "x87 Floating-Point Exception") && eq(exception_name(17), "Alignment Check") && eq(exception_name(19), "SIMD Floating-Point Exception") && eq(exception_name(21), "Control Protection Exception"),
          "got \"%s\", \"%s\", \"%s\", \"%s\"", exception_name(16), exception_name(17), exception_name(19), exception_name(21));
    CHECK("exception_name: reserved and external vectors", eq(exception_name(15), "Reserved") && eq(exception_name(22), "Reserved") && eq(exception_name(31), "Reserved") && eq(exception_name(32), "External Interrupt") && eq(exception_name(255), "External Interrupt"),
          "got \"%s\", \"%s\", \"%s\", \"%s\"", exception_name(15), exception_name(22), exception_name(31), exception_name(32));
    int mask_ok = 1;
    for (unsigned v = 0; v < 40; v++) {
        int want = v == 8 || v == 10 || v == 11 || v == 12 || v == 13 || v == 14 || v == 17 || v == 21 || v == 29 || v == 30;
        if (exception_has_error_code(v) != want) mask_ok = 0;
    }
    CHECK("exception_has_error_code is right for vectors 0 to 39", mask_ok, "at least one vector is wrong");

    PF(0, "read from a non-present page from kernel mode");
    PF(2, "write to a non-present page from kernel mode");
    PF(1, "read from a protected page from kernel mode");
    PF(7, "write to a protected page from user mode");
    PF(4, "read from a non-present page from user mode");
    PF(0x11, "fetch from a protected page from kernel mode");
    PF(0x0F, "write to a protected page from user mode, reserved bit set");
    PF(0x18, "fetch from a non-present page from kernel mode, reserved bit set");

    memset(buf, '#', sizeof buf);
    describe_page_fault(2, buf, 10);
    CHECK("describe_page_fault never writes beyond cap", eq(buf, "write to ") && buf[10] == '#', "got \"%.12s\"", buf);

    memset(buf, '#', sizeof buf);
    describe_selector_error(0, buf, sizeof buf);
    CHECK("describe_selector_error: error code 0 has no selector", eq(buf, "no selector"), "got \"%s\"", buf);
    describe_selector_error(0x28, buf, sizeof buf);
    CHECK("describe_selector_error: GDT index 5", eq(buf, "GDT index 5"), "got \"%s\"", buf);
    describe_selector_error((13 << 3) | 2, buf, sizeof buf);
    CHECK("describe_selector_error: IDT index 13", eq(buf, "IDT index 13"), "got \"%s\"", buf);
    describe_selector_error((13 << 3) | 4, buf, sizeof buf);
    CHECK("describe_selector_error: table value 2 is the LDT", eq(buf, "LDT index 13"), "got \"%s\"", buf);
    describe_selector_error((13 << 3) | 6, buf, sizeof buf);
    CHECK("describe_selector_error: table value 3 is also the IDT", eq(buf, "IDT index 13"), "got \"%s\"", buf);
    describe_selector_error((200 << 3) | 7, buf, sizeof buf);
    CHECK("describe_selector_error: the IDT with the external bit", eq(buf, "IDT index 200 (external)"), "got \"%s\"", buf);
    describe_selector_error(0x08, buf, sizeof buf);
    CHECK("describe_selector_error: selector 0x08", eq(buf, "GDT index 1"), "got \"%s\"", buf);
    describe_selector_error(1, buf, sizeof buf);
    CHECK("describe_selector_error: external with index 0", eq(buf, "GDT index 0 (external)"), "got \"%s\"", buf);

    format_crash(buf, sizeof buf, 3, 0, 0x101234, 0);
    CHECK("format_crash: a breakpoint", eq(buf, "Breakpoint (vector 3) at 0x101234"), "got \"%s\"", buf);
    format_crash(buf, sizeof buf, 14, 2, 0x101234, 0);
    CHECK("format_crash: a page fault", eq(buf, "Page Fault (vector 14) at 0x101234: write to a non-present page from kernel mode, address 0x0"), "got \"%s\"", buf);
    format_crash(buf, sizeof buf, 14, 7, 0xFFFF800000001000UL, 0xDEAD000);
    CHECK("format_crash: a user page fault with a 64-bit rip", eq(buf, "Page Fault (vector 14) at 0xffff800000001000: write to a protected page from user mode, address 0xdead000"), "got \"%s\"", buf);
    format_crash(buf, sizeof buf, 13, 0x28, 0x2000, 0);
    CHECK("format_crash: a general protection fault with a selector", eq(buf, "General Protection Fault (vector 13) at 0x2000: GDT index 5"), "got \"%s\"", buf);
    format_crash(buf, sizeof buf, 13, 0, 0x2000, 0);
    CHECK("format_crash: a general protection fault with error code 0 says nothing more", eq(buf, "General Protection Fault (vector 13) at 0x2000"), "got \"%s\"", buf);
    format_crash(buf, sizeof buf, 8, 0, 0x2000, 0);
    CHECK("format_crash: a double fault", eq(buf, "Double Fault (vector 8) at 0x2000"), "got \"%s\"", buf);
    char tiny[12];
    memset(tiny, '#', sizeof tiny);
    format_crash(tiny, 10, 3, 0, 0x1000, 0);
    CHECK("format_crash respects cap", eq(tiny, "Breakpoi") == 0 && tiny[9] == 0 && tiny[10] == '#', "got \"%.12s\"", tiny);

    CHECK("crash_last_report is empty before any exception", eq(crash_last_report(), ""), "got \"%s\"", crash_last_report());
    crash_init();
    __asm__ volatile("int3");
    const char *r = crash_last_report();
    CHECK("the live handler reports a real breakpoint", starts(r, "Breakpoint (vector 3) at 0x") && r[27] != 0, "report is \"%s\"", r);
    unsigned long rip = 0;
    const char *h = r + 27;
    while ((*h >= '0' && *h <= '9') || (*h >= 'a' && *h <= 'f')) { rip = rip * 16 + (unsigned long)(*h <= '9' ? *h - '0' : *h - 'a' + 10); h++; }
    CHECK("the reported instruction pointer is inside the kernel image", rip >= 0x100000 && rip < 0x4000000 && *h == 0, "report is \"%s\"", r);
}
