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
    return "";
}

int exception_has_error_code(unsigned vector) {
    return 0;
}

void describe_page_fault(uint64_t err, char *out, size_t cap) {
}

void describe_selector_error(uint64_t err, char *out, size_t cap) {
}

void format_crash(char *out, size_t cap, uint64_t vec, uint64_t err, uint64_t rip, uint64_t cr2) {
}

void crash_init(void) {
}

const char *crash_last_report(void) {
    return "";
}
