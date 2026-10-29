#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

// Provided: sends one character to the serial port (COM1, already configured).
void serial_putc(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0) {
    }
    outb(0x3F8, (uint8_t)c);
}

typedef struct { char *buf; size_t size, count; } Out;

static void emit(Out *o, char c) {
    if (o->count + 1 < o->size) o->buf[o->count] = c;
    o->count++;
}

static void pad(Out *o, int n, char c) {
    while (n-- > 0) emit(o, c);
}

static void emit_number(Out *o, uint64_t mag, int neg, int base, int width, int zero) {
    char digits[24];
    int i = 24;
    do {
        digits[--i] = "0123456789abcdef"[mag % (uint64_t)base];
        mag /= (uint64_t)base;
    } while (mag);
    int len = 24 - i + neg;
    if (!zero) pad(o, width - len, ' ');
    if (neg) emit(o, '-');
    if (zero) pad(o, width - len, '0');
    for (; i < 24; i++) emit(o, digits[i]);
}

static int kvsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    Out o = {buf, size, 0};
    for (; *fmt; fmt++) {
        if (*fmt != '%') { emit(&o, *fmt); continue; }
        const char *start = fmt++;
        int zero = 0, width = 0, lng = 0;
        if (*fmt == '0') { zero = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        if (*fmt == 'l') { lng = 1; fmt++; }
        switch (*fmt) {
        case '%': emit(&o, '%'); break;
        case 'c': pad(&o, width - 1, ' '); emit(&o, (char)va_arg(ap, int)); break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int n = 0;
            while (s[n]) n++;
            pad(&o, width - n, ' ');
            for (; *s; s++) emit(&o, *s);
            break;
        }
        case 'd': {
            long v = lng ? va_arg(ap, long) : va_arg(ap, int);
            int neg = v < 0;
            emit_number(&o, neg ? -(uint64_t)v : (uint64_t)v, neg, 10, width, zero);
            break;
        }
        case 'u': emit_number(&o, lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int), 0, 10, width, zero); break;
        case 'x': emit_number(&o, lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int), 0, 16, width, zero); break;
        case 'p':
            emit(&o, '0'); emit(&o, 'x');
            emit_number(&o, (uint64_t)(uintptr_t)va_arg(ap, void *), 0, 16, 0, 0);
            break;
        default:
            for (const char *p = start; p <= fmt && *p; p++) emit(&o, *p);
            if (!*fmt) fmt--;
            break;
        }
    }
    if (size) buf[o.count < size ? o.count : size - 1] = 0;
    return (int)o.count;
}

int ksnprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = kvsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}

int kprintf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = kvsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    for (int i = 0; buf[i]; i++) serial_putc(buf[i]);
    return n;
}

void kmain(void) {
    kprintf("Hello %s, you are number %d (0x%x)\r\n", "kernel", 42, 42);
}
