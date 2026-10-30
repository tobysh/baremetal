---
title: Bit Masks
summary: Use a one-bit mask to set, clear, test and toggle a single bit of a number, the building block of flags, permissions and hardware registers.
minutes: 15
difficulty: easy
tags: bitwise, flags
id: c-memory/bit-masks
mode: harness
---
A 32-bit number is 32 tiny on/off switches side by side. The rightmost is **bit 0**, next is bit 1, and so on up to bit 31. Programs use them as flags: bit 0 might mean "readable", bit 1 "writable", bit 2 "executable".

## Building a mask

A **mask** is a number with a 1 in just the position you care about. You make one by shifting 1 left:

```c
1u << 0    // 0000 0001   (bit 0)
1u << 3    // 0000 1000   (bit 3)
1u << 5    // 0010 0000   (bit 5)
```

The `u` makes the 1 unsigned, which keeps `1u << 31` well defined.

## The four moves

Take `x = 0b1010` (decimal 10) and the mask `m = 1u << 2` (`0b0100`):

| Goal | Expression | Result |
|------|-----------|--------|
| **Set** the bit (turn on) | `x \| m` | `1110` |
| **Clear** the bit (turn off) | `x & ~m` | `1010` |
| **Toggle** the bit (flip) | `x ^ m` | `1110` |
| **Test** the bit | `(x & m) != 0` | `0` (bit 2 is off) |

Why each one works:

- OR with a 1 forces the bit on; OR with a 0 leaves the other bits alone.
- `~m` flips the mask so it has a 0 in your position and 1s everywhere else. AND-ing keeps every bit except yours.
- XOR with a 1 flips; XOR with a 0 changes nothing.
- AND with the mask hides every bit but yours. The result is non-zero exactly when your bit was on.

## Functions that return a new value

C passes numbers by value, so these helpers take `x` and return the changed number. The caller writes `flags = set_bit(flags, 3);`.

## Common mistakes

- Using `&&` or `||` (logical) where you need `&` or `|` (bitwise).
- Writing `x & m == 0`; `==` binds tighter than `&`. Write `(x & m) == 0`.
- Using `1 << n` with a signed int when `n` reaches 31.
- Forgetting that bit numbering starts at 0.

## Your turn

Write the four helpers. Every bit position from 0 to 31 is tried.
