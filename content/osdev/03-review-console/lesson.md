---
title: "Review: A Two-Screen Console"
summary: A consolidation exercise. Send text to the serial port and the screen at once, and print numbers in decimal and hex by hand. No new hardware, just practice.
minutes: 35
difficulty: medium
kind: review
tags: hardware, io, serial, vga
id: osdev/review-console
mode: harness
---
This is a **review lesson**. It introduces no new hardware. You have driven the serial port with port I/O and the VGA text screen with memory-mapped I/O. Before the course moves on to the CPU's own tables, this exercise puts the two together into something every kernel needs: a **console**.

## What you already have

The starter contains your two drivers, ready to use:

- `serial_init`, `serial_putc` and `serial_puts` from the serial lesson. Remember that `serial_puts` turns `\n` into `\r\n`, because a serial terminal needs both a carriage return and a line feed.
- `term_init`, `term_set_color`, `term_putc` and the scrolling and cursor code from the VGA lesson. `term_putc` already understands `\n`, `\r`, `\b` and `\t`.

Read them over. They are the answers to the first two lessons, so this is also a chance to see whether the ideas still make sense to you.

## Why a console?

Real kernels rarely talk to one device. Early in boot the screen may not be usable yet, and later the screen may be nobody's idea of a log file, so most kernels **mirror** their output: everything goes to the serial port (which an emulator or a logging machine can capture) and to the screen (which a person can read). One function, `console_putc`, hides that from the rest of the kernel.

There is a subtle point. The serial port and the screen disagree about what a newline is:

- To the **screen**, `\n` means "go to the start of the next row". `term_putc` already handles it.
- To the **serial port**, `\n` only means "move down". You must send `\r` first to get back to the left edge. `serial_puts` did this for whole strings, but when you are sending one character at a time, `console_putc` has to do it itself.

So `console_putc(c)` is: if `c` is `'\n'`, send `'\r'` to the serial port first; then send `c` to the serial port; then send `c` to the screen. Do **not** send the extra `'\r'` to the screen as well, or the cursor would jump to column 0 twice.

## Printing numbers

A kernel has no `printf` (the test runner's own `bm_printf` is not part of your kernel). To show a number you must turn it into characters yourself. Two small functions cover most needs.

### Decimal

The last decimal digit of `n` is `n % 10`, and `n / 10` is everything before it. That produces the digits **backwards**: least significant first. The usual fix is a small buffer filled from the end:

```c
char buf[20];            // a 64-bit number has at most 20 decimal digits
int i = 20;
do {
    buf[--i] = (char)('0' + n % 10);
    n /= 10;
} while (n != 0);
// buf[i..20) now holds the digits in the right order
```

The `do { } while` matters. A plain `while (n != 0)` would print nothing at all for `0`, and `0` should print as `"0"`.

### Hexadecimal

Hex is easier because 16 is a power of two: each hex digit is exactly **four bits**. The lowest digit is `n & 0xF`, and `n >> 4` drops it. Map 0 to 9 to `'0'`-`'9'` and 10 to 15 to `'a'`-`'f'`. Print a `0x` prefix, use lowercase letters, and use no leading zeros, except that `0` prints as `0x0`.

Both functions must print through `console_putc`, so that digits appear on the serial port and the screen alike.

## Things to watch for

- Make the loop work for **all 64 bits**. The largest `uint64_t`, 18446744073709551615, is 20 digits, and `0xffffffffffffffff` is 16 hex digits. A buffer that is too small writes over your own stack, and in a kernel nothing will tell you.
- Use `uint64_t`, not `int`, for the arithmetic. Dividing a signed 32-bit number would silently give the wrong answer for big values.
- Write the number-printing functions once each, and have `console_puts` just loop over `console_putc`. Less code means fewer places for a bug.
