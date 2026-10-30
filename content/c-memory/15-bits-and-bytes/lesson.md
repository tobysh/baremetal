---
title: Bits and Bytes
summary: See numbers as the bits the hardware stores. Learn binary, the bitwise operators & | ^ ~ << >>, and how endianness decides the order of bytes in memory.
minutes: 30
difficulty: hard
tags: bitwise
id: c-memory/bits-and-bytes
mode: harness
---
Every house on the memory street holds a **byte**, and a byte is 8 **bits**, tiny switches that are either 0 or 1. Everything in a computer, numbers, text, pictures, is stored as bits.

## Binary

Decimal uses ten digits and each place is worth 10 times the one before. **Binary** uses two digits and each place is worth 2 times the one before: 1, 2, 4, 8, 16, ...

```
binary 1011  =  1*8 + 0*4 + 1*2 + 1*1  =  11
```

**Hexadecimal** (base 16, written with a `0x` prefix) is a compact way to write binary: one hex digit is exactly 4 bits. So `0xB` is `1011`, and `0x12345678` is the 32-bit number made of the bytes `12`, `34`, `56`, `78`.

A `uint32_t` (from `<stdint.h>`) is an unsigned integer of exactly 32 bits. We use unsigned types for bit work because shifting signed negative values is murky territory in C.

## Bitwise operators

These work on every bit position independently:

| Operator | Meaning | Example (4 bits) |
|----------|---------|------------------|
| `a & b` | AND: 1 only if both are 1 | `1100 & 1010 = 1000` |
| `a \| b` | OR: 1 if either is 1 | `1100 \| 1010 = 1110` |
| `a ^ b` | XOR: 1 if they differ | `1100 ^ 1010 = 0110` |
| `~a` | NOT: flip every bit | `~1100 = 0011` |
| `a << n` | shift left n places (multiply by 2^n) | `0011 << 2 = 1100` |
| `a >> n` | shift right n places (divide by 2^n) | `1100 >> 2 = 0011` |

Don't confuse `&` with `&&`. `&&` asks a yes/no question about whole values; `&` combines bits.

## Useful tricks

```c
x & 1               // the lowest bit: 1 if x is odd
(x >> 3) & 1        // bit number 3
x | (1u << 5)       // turn bit 5 on
x & ~(1u << 5)      // turn bit 5 off
x & (x - 1)         // clear the lowest set bit
```

That last one is gold. Subtracting 1 flips the lowest 1 bit to 0 and the zeros below it to 1s, so AND-ing wipes that bit out. Repeat it until `x` is 0 and the number of repeats is the number of 1 bits. A power of two has exactly one 1 bit, so `x & (x - 1)` is 0 for it.

## Masks and shifts to pick out bytes

To grab the second byte of a 32-bit value, shift it down and mask off the rest: `(x >> 8) & 0xFF`. To put a byte back in position, shift it left: `b << 8`.

## Endianness

A 32-bit number takes four bytes, so which byte goes at the lowest address? **Little-endian** machines (x86, most phones) store the least significant byte first; **big-endian** machines (and most network protocols) store the most significant first. The number `0x12345678` is stored as `78 56 34 12` on a little-endian machine. When data crosses between the two, you must reverse the byte order, a **byte swap**.

## Common mistakes

- Writing `1 << 31` with a signed `int` (overflow). Use `1u << 31`.
- Shifting by the type's width or more, which is undefined behaviour.
- Forgetting operator precedence: `x & 1 == 0` means `x & (1 == 0)`. Parenthesise: `(x & 1) == 0`.
