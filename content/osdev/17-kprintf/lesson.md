---
title: Formatted Output: Your Own printf
summary: Build ksnprintf, a bounds-checked formatter with %s, %c, %d, %u, %x, %p, width and zero padding, using variable arguments and no libc.
minutes: 50
difficulty: medium
tags: c, strings, varargs
id: osdev/kprintf
mode: harness
---
Printing numbers one function at a time (`kprint_dec`, `kprint_hex`) works, but a kernel's log lines look like `"page fault at 0x%lx (error %d) in task %s"`. You need `printf`. There is no libc in a kernel, so you write it. This lesson builds the version that writes into a **buffer** (`snprintf`), because the buffer version is the safe one and every other output function can be built on top of it: format into a buffer, then send the buffer to the serial port or the screen.

## Variable arguments

A function like `ksnprintf(char *buf, size_t size, const char *fmt, ...)` takes any number of extra arguments. C gives you macros from `<stdarg.h>`, which is available even in a freestanding kernel because it's a compiler feature, not a library:

```c
#include <stdarg.h>

va_list ap;
va_start(ap, fmt);           // ap now points just after fmt
int n = va_arg(ap, int);     // fetch the next argument, as an int
char *s = va_arg(ap, char *);
va_end(ap);
```

You have to tell `va_arg` the exact type. Fetching an `int` when the caller passed a `long` is undefined behaviour; this is why the format string has to say `%d` or `%ld`. Small integer types and `char` are always promoted to `int` when passed through `...`, so `%c` fetches an `int`.

## What to support

| Conversion | Meaning |
|---|---|
| `%c` | one character |
| `%s` | a string (`NULL` prints `(null)`) |
| `%d` | signed decimal; `%ld` takes a `long` |
| `%u` | unsigned decimal; `%lu` takes an `unsigned long` |
| `%x` | lower-case hex, no `0x` prefix; `%lx` takes an `unsigned long` |
| `%p` | a pointer: `0x` followed by lower-case hex |
| `%%` | a literal percent sign |

Between the `%` and the letter you may have a `0` flag and a decimal **width**: `%5d` pads with spaces on the left to at least 5 characters, `%05d` pads with zeros. With a negative number, zero padding goes *after* the sign: `%05d` of `-42` is `-0042`. Strings are padded with spaces only. A width never truncates.

An unknown conversion is copied through unchanged (`%q` prints `%q`).

## The `snprintf` contract

The caller says how big the buffer is, and you must never write past it. The rules, taken from C:

- Write at most `size - 1` characters, then a `'\0'`. If `size` is 0, write nothing at all.
- **Always** return the length the full output would have had, even when it was cut short. Callers compare the return value with `size` to detect truncation, and a call like `ksnprintf(NULL, 0, ...)` measures the length first.

The neat way to get both is a tiny "emit" helper that counts every character but only stores the ones that fit:

```c
typedef struct { char *buf; size_t size, count; } Out;

static void emit(Out *o, char c) {
    if (o->count + 1 < o->size) o->buf[o->count] = c;
    o->count++;
}
```

At the end, terminate with `if (o.size) o.buf[o.count < o.size ? o.count : o.size - 1] = 0;`.

## Converting a number

You did this in the console review: repeatedly take `n % base` and divide, filling a small array from the end, then print it forwards. The extras here are the sign (print `-` for negatives, and be careful with the most negative value, which can't be negated in a signed type; negate in an unsigned type instead) and the padding, which you can work out because you know the digit count before printing anything.

## Debugging checklist

- **A garbled number follows `%ld`:** you fetched an `int` instead of a `long`.
- **The buffer is overrun:** every store must go through the bounds check in `emit`.
- **The return value shrinks when the buffer is small:** it must count everything, stored or not.
- **`-2147483648` prints wrong:** don't write `-n` on a signed `int`; use an unsigned magnitude.

## Your turn

Write `ksnprintf` in one function plus helpers. The hidden tests format dozens of strings and also check the bytes just beyond the buffer for damage.
