---
title: Integer Arithmetic Without Surprises
summary: Choose between checked, wrapping, saturating and overflowing arithmetic, cast safely between integer types and use bit operations.
minutes: 25
difficulty: medium
tags: integers, overflow, bit-manipulation
id: rust-bridge/integer-arithmetic
mode: harness
---
In C, signed overflow is undefined behaviour and unsigned overflow silently wraps. Rust makes you say what you mean. In a debug build, plain `a + b` **panics** on overflow; in a release build it wraps. Never rely on either: pick a method.

| Method | On overflow |
|---|---|
| `a.checked_add(b)` | returns `None` |
| `a.wrapping_add(b)` | wraps around (like C unsigned) |
| `a.saturating_add(b)` | sticks at `MIN` or `MAX` |
| `a.overflowing_add(b)` | returns `(wrapped, did_overflow)` |

The same family exists for `sub`, `mul`, `pow`, `neg`. `abs()` of `i32::MIN` overflows too: use `checked_abs` or `unsigned_abs`.

## Casting with `as`

`as` never fails; it truncates or reinterprets:

```rust
300_i32 as u8     // 44   (keeps the low 8 bits)
-1_i32 as u32     // 4294967295
3.9_f64 as i32    // 3    (floats saturate, NaN becomes 0)
```

When you want to know if a conversion is lossless, use `TryFrom`: `u8::try_from(300_i32)` is `Err(..)`, and `u8::try_from(200_i32)` is `Ok(200)`. `i64::from(x_i32)` always works and is lossless.

## Bits

```rust
x & mask      // and          x | mask   // or          x ^ mask   // xor
x << 3        // shift left   x >> 3     // shift right (arithmetic for signed)
!x            // bitwise not (it is ! in Rust, not ~)
x.count_ones(), x.leading_zeros(), x.trailing_zeros(), x.rotate_left(5)
```

Shifting by the type's width or more panics in debug builds, so keep shift amounts below the bit count.

## This lesson's exercise

Hashing, factorials, clamping and bit tricks, each with the overflow policy spelled out.
