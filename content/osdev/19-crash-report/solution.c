#include <stdint.h>
#include <stddef.h>

extern void (*bm_exception_hook)(uint64_t vector, uint64_t error_code, uint64_t rip);

// ---- Provided helpers: append to `out` at position `pos`, never past `cap - 1` characters,
// ---- always leaving a NUL at the end. They return the new length.
static size_t put_str(char *out, size_t cap, size_t pos, const char *s) {
    while (*s && pos + 1 < cap) out[pos++] = *s++;
    if (cap) out[pos] = 0;
    return pos;
}

static size_t put_dec(char *out, size_t cap, size_t pos, uint64_t v) {
    char tmp[21];
    int i = 21;
    tmp[--i] = 0;
    do { tmp[--i] = (char)('0' + v % 10); v /= 10; } while (v);
    return put_str(out, cap, pos, tmp + i);
}

// Appends "0x" and the value in lower-case hex.
static size_t put_hex(char *out, size_t cap, size_t pos, uint64_t v) {
    char tmp[19];
    int i = 19;
    tmp[--i] = 0;
    do { tmp[--i] = "0123456789abcdef"[v & 15]; v >>= 4; } while (v);
    tmp[--i] = 'x';
    tmp[--i] = '0';
    return put_str(out, cap, pos, tmp + i);
}

// ---- Your code ----

const char *exception_name(unsigned vector) {
    static const char *const names[22] = {
        "Divide Error", "Debug", "Non-Maskable Interrupt", "Breakpoint", "Overflow",
        "Bound Range Exceeded", "Invalid Opcode", "Device Not Available", "Double Fault",
        "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present", "Stack-Segment Fault",
        "General Protection Fault", "Page Fault", "Reserved", "x87 Floating-Point Exception",
        "Alignment Check", "Machine Check", "SIMD Floating-Point Exception",
        "Virtualization Exception", "Control Protection Exception"};
    if (vector < 22) return names[vector];
    return vector < 32 ? "Reserved" : "External Interrupt";
}

int exception_has_error_code(unsigned vector) {
    switch (vector) {
    case 8: case 10: case 11: case 12: case 13: case 14: case 17: case 21: case 29: case 30:
        return 1;
    default:
        return 0;
    }
}

void describe_page_fault(uint64_t err, char *out, size_t cap) {
    size_t pos = put_str(out, cap, 0, "");
    pos = put_str(out, cap, pos, (err & 16) ? "fetch from" : (err & 2) ? "write to" : "read from");
    pos = put_str(out, cap, pos, " a ");
    pos = put_str(out, cap, pos, (err & 1) ? "protected" : "non-present");
    pos = put_str(out, cap, pos, " page from ");
    pos = put_str(out, cap, pos, (err & 4) ? "user" : "kernel");
    pos = put_str(out, cap, pos, " mode");
    if (err & 8) put_str(out, cap, pos, ", reserved bit set");
}

void describe_selector_error(uint64_t err, char *out, size_t cap) {
    size_t pos = put_str(out, cap, 0, "");
    if (err == 0) {
        put_str(out, cap, pos, "no selector");
        return;
    }
    static const char *const tables[4] = {"GDT", "IDT", "LDT", "IDT"};
    pos = put_str(out, cap, pos, tables[(err >> 1) & 3]);
    pos = put_str(out, cap, pos, " index ");
    pos = put_dec(out, cap, pos, err >> 3);
    if (err & 1) put_str(out, cap, pos, " (external)");
}

void format_crash(char *out, size_t cap, uint64_t vec, uint64_t err, uint64_t rip, uint64_t cr2) {
    size_t pos = put_str(out, cap, 0, "");
    pos = put_str(out, cap, pos, exception_name((unsigned)vec));
    pos = put_str(out, cap, pos, " (vector ");
    pos = put_dec(out, cap, pos, vec);
    pos = put_str(out, cap, pos, ") at ");
    pos = put_hex(out, cap, pos, rip);
    char tmp[96];
    if (vec == 14) {
        describe_page_fault(err, tmp, sizeof tmp);
        pos = put_str(out, cap, pos, ": ");
        pos = put_str(out, cap, pos, tmp);
        pos = put_str(out, cap, pos, ", address ");
        put_hex(out, cap, pos, cr2);
    } else if (vec >= 10 && vec <= 13 && err != 0) {
        describe_selector_error(err, tmp, sizeof tmp);
        pos = put_str(out, cap, pos, ": ");
        put_str(out, cap, pos, tmp);
    }
}

static char last_report[160];

static void crash_handler(uint64_t vector, uint64_t error_code, uint64_t rip) {
    uint64_t cr2;
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    format_crash(last_report, sizeof last_report, vector, error_code, rip, cr2);
}

void crash_init(void) {
    bm_exception_hook = crash_handler;
}

const char *crash_last_report(void) {
    return last_report;
}
