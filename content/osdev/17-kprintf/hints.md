Structure it as a counting output sink. `emit(o, c)` stores `c` only when `o->count + 1 < o->size` and always does `o->count++`. Every character the function produces, literal text and digits alike, goes through `emit`. That one rule gives you both safety and the right return value.
===
Write `static void emit_number(Out *o, uint64_t mag, int neg, int base, int width, int zero)`. Fill a `char digits[24]` from the end with `do { digits[--i] = "0123456789abcdef"[mag % base]; mag /= base; } while (mag);`. Then `len = 24 - i + neg`. Padding spaces go before the sign; padding zeros go after it.
===
Walk `fmt` with a `for (; *fmt; fmt++)` loop. After a `%`, parse the flag (`'0'`), then the width digits (`width = width * 10 + (*fmt - '0')`), then an optional `'l'`, then switch on the conversion. For `%d`, fetch `long v = lng ? va_arg(ap, long) : va_arg(ap, int);`, then `neg = v < 0; mag = neg ? -(uint64_t)v : (uint64_t)v;`.
===
`%u` and `%x` fetch `unsigned long` when `l` was given, else `unsigned int`. `%p` is `emit '0'`, `emit 'x'`, then a hex number from `(uintptr_t)va_arg(ap, void *)` with no padding. Strings: compute `strlen`, pad with spaces up to the width, then emit each character; `NULL` becomes `"(null)"`.
===
Structure of the main function:
```c
int ksnprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    Out o = {buf, size, 0};
    for (; *fmt; fmt++) {
        if (*fmt != '%') { emit(&o, *fmt); continue; }
        const char *start = fmt++;
        /* flag, width, 'l', then switch (*fmt) { ... default: copy start..fmt } */
    }
    if (size) buf[o.count < size ? o.count : size - 1] = 0;
    va_end(ap);
    return (int)o.count;
}
```
For `kprintf`, a `va_list` cannot be passed to `ksnprintf` as `...`. Move the body into `kvsnprintf(char *, size_t, const char *, va_list)` and have both public functions call it.
