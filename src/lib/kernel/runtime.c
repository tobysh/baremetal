/* Hidden runtime for the "kernel" language: the pieces a kernel normally gets
 * from nowhere. The test channel is the QEMU debug console (port 0xE9), kept
 * separate from COM1 so the learner's serial driver can't interfere with it. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef unsigned long size_t;

void outb(u16 port, u8 v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
u8 inb(u16 port) { u8 v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port)); return v; }
void outw(u16 port, u16 v) { __asm__ volatile("outw %0, %1" : : "a"(v), "Nd"(port)); }
u16 inw(u16 port) { u16 v; __asm__ volatile("inw %1, %0" : "=a"(v) : "Nd"(port)); return v; }
void outl(u16 port, u32 v) { __asm__ volatile("outl %0, %1" : : "a"(v), "Nd"(port)); }
u32 inl(u16 port) { u32 v; __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(port)); return v; }

__attribute__((weak)) void *memset(void *d, int c, size_t n) { u8 *p = d; while (n--) *p++ = (u8)c; return d; }
__attribute__((weak)) void *memcpy(void *d, const void *s, size_t n) { u8 *p = d; const u8 *q = s; while (n--) *p++ = *q++; return d; }
__attribute__((weak)) void *memmove(void *d, const void *s, size_t n) {
    u8 *p = d; const u8 *q = s;
    if (p < q) while (n--) *p++ = *q++;
    else { p += n; q += n; while (n--) *--p = *--q; }
    return d;
}
__attribute__((weak)) int memcmp(const void *a, const void *b, size_t n) {
    const u8 *p = a, *q = b;
    for (; n; n--, p++, q++) if (*p != *q) return *p - *q;
    return 0;
}
__attribute__((weak)) size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }

/* Leaves QEMU with status (code << 1) | 1. */
void bm_exit(int code) {
    outl(0xF4, (u32)code);
    for (;;) __asm__ volatile("hlt");
}

void bm_putc(char c) { outb(0xE9, (u8)c); }
static void puts_n(const char *s, int n) { while (n-- > 0 && *s) bm_putc(*s++); }

/* A small printf for test output: %s %c %d %i %u %x %X %p %%, with 0 flag,
 * width, .precision / .* for strings, and l / ll / z length modifiers. */
void bm_vprintf(const char *f, __builtin_va_list ap) {
    for (; *f; f++) {
        if (*f != '%') { bm_putc(*f); continue; }
        f++;
        int zero = 0, width = 0, prec = -1, lng = 0;
        if (*f == '0') { zero = 1; f++; }
        while (*f >= '0' && *f <= '9') width = width * 10 + (*f++ - '0');
        if (*f == '.') {
            f++;
            if (*f == '*') { prec = __builtin_va_arg(ap, int); f++; }
            else { prec = 0; while (*f >= '0' && *f <= '9') prec = prec * 10 + (*f++ - '0'); }
        }
        while (*f == 'l' || *f == 'z') { lng = 1; f++; }
        char buf[24]; int n = 0; const char *s = buf; int neg = 0;
        switch (*f) {
        case '%': bm_putc('%'); continue;
        case 'c': buf[0] = (char)__builtin_va_arg(ap, int); n = 1; break;
        case 's': {
            s = __builtin_va_arg(ap, const char *);
            if (!s) s = "(null)";
            n = (int)strlen(s);
            if (prec >= 0 && n > prec) n = prec;
            break;
        }
        case 'd': case 'i': case 'u': case 'x': case 'X': case 'p': {
            u64 v; int base = 10; int upper = *f == 'X';
            if (*f == 'x' || *f == 'X') base = 16;
            if (*f == 'p') { base = 16; lng = 1; bm_putc('0'); bm_putc('x'); }
            if (*f == 'd' || *f == 'i') {
                long sv = lng ? __builtin_va_arg(ap, long) : __builtin_va_arg(ap, int);
                neg = sv < 0; v = neg ? -(u64)sv : (u64)sv;
            } else v = lng ? __builtin_va_arg(ap, u64) : __builtin_va_arg(ap, u32);
            char tmp[24]; int t = 0;
            do { int d = (int)(v % base); tmp[t++] = (char)(d < 10 ? '0' + d : (upper ? 'A' : 'a') + d - 10); v /= base; } while (v);
            if (neg) buf[n++] = '-';
            while (t) buf[n++] = tmp[--t];
            break;
        }
        default: bm_putc('%'); bm_putc(*f); continue;
        }
        for (int i = n; i < width; i++) bm_putc(zero && *f != 's' ? '0' : ' ');
        puts_n(s, n);
    }
}
void bm_printf(const char *f, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, f);
    bm_vprintf(f, ap);
    __builtin_va_end(ap);
}

/* ---- default exception reporting ---- */
extern void *bm_isr_table[48];
struct idt_gate { u16 lo; u16 sel; u8 ist; u8 type; u16 mid; u32 hi; u32 zero; } __attribute__((packed));
static struct idt_gate boot_idt[32];
extern u32 bm_multiboot_info, bm_multiboot_magic;

void bm_runtime_init(void) {
    for (int i = 0; i < 32; i++) {
        u64 a = (u64)bm_isr_table[i];
        boot_idt[i] = (struct idt_gate){(u16)a, 0x08, 0, 0x8E, (u16)(a >> 16), (u32)(a >> 32), 0};
    }
    struct { u16 size; u64 base; } __attribute__((packed)) p = {sizeof boot_idt - 1, (u64)boot_idt};
    __asm__ volatile("lidt %0" : : "m"(p));
}

static const char *const exc_name[32] = {
    "divide error (#DE)", "debug (#DB)", "NMI", "breakpoint (#BP)", "overflow (#OF)", "bound range (#BR)",
    "invalid opcode (#UD)", "device not available (#NM)", "double fault (#DF)", "coprocessor overrun",
    "invalid TSS (#TS)", "segment not present (#NP)", "stack fault (#SS)", "general protection fault (#GP)",
    "page fault (#PF)", "reserved", "x87 error (#MF)", "alignment check (#AC)", "machine check (#MC)",
    "SIMD error (#XM)", "virtualization (#VE)", "control protection (#CP)", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved", "reserved", "reserved", "reserved", "reserved"};

/* A learner-installed handler. If set, exceptions and IRQs go to it and execution resumes
 * afterwards (iretq). If not, an exception is reported and the machine stops. */
void (*bm_exception_hook)(u64 vector, u64 error_code, u64 rip);

void bm_exception(u64 vec, u64 err, u64 rip, u64 cr2) {
    bm_printf("\n@@EXC %s\trip=0x%lx err=0x%lx cr2=0x%lx\n", exc_name[vec & 31], rip, err, cr2);
    bm_exit(3);
}

void bm_dispatch(u64 vec, u64 err, u64 rip, u64 cr2) {
    if (bm_exception_hook) { bm_exception_hook(vec, err, rip); return; }
    bm_exception(vec, err, rip, cr2);
}
