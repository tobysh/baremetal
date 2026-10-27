---
title: Bit Twiddling
summary: Shift, mask and clear bits with and, shr and sub to count the set bits in a 64-bit number, and learn which registers you must preserve.
minutes: 25
difficulty: medium
tags: bitwise, loops
id: asm-first-steps/popcount
mode: harness
---
Registers are 64 bits wide, and assembly gives you direct tools for working on those bits one at a time.

## Bitwise instructions

| Instruction | C | Effect |
|---|---|---|
| `and a, b` | `a &= b` | keep bits set in both |
| `or a, b` | `a \|= b` | set bits set in either |
| `xor a, b` | `a ^= b` | flip bits where `b` is 1 |
| `not a` | `a = ~a` | invert every bit |
| `shl a, n` | `a <<= n` | shift left, filling with zeros |
| `shr a, n` | `a >>= n` | shift right, filling with zeros (unsigned) |
| `sar a, n` | `a >>= n` | shift right, copying the sign bit (signed) |

All of them set `ZF` if the result is zero, so you can `jz` straight after.

The shift count is an immediate or the register `cl`: `shr rax, cl`.

### Shifts, with a picture

A shift slides every bit sideways. Using 8 bits:

```
value      1011 0100
shl 1      0110 1000     top bit falls off (into CF), a 0 enters at the bottom
shr 1      0101 1010     bottom bit falls off (into CF), a 0 enters at the top
sar 1      1101 1010     bottom bit falls off, the old top bit is copied back in
```

`shl n` multiplies by 2^n and `shr n` divides an unsigned number by 2^n. `sar` is the signed divide: it keeps negative numbers negative. The bit that falls off is not lost for free: it lands in `CF`, so `shr rax, 1` followed by `jc` branches on what the lowest bit *was*.

### Masks and constants

To test, set or clear specific bits you combine the value with a **mask**:

```asm
and rax, 0xFF        # keep only the low 8 bits (clear the rest)
or  rax, 0x10        # set bit 4
and rax, ~0x10       # clear bit 4 (the assembler computes the inversion)
xor rax, 0x10        # toggle bit 4
test rax, 0x10       # is bit 4 set? ZF = 0 if yes
```

There is a catch with big constants. Most instructions only accept a **32-bit** immediate, and for 64-bit operations it is *sign-extended*. So `and rax, 0x80000000` does not mean what it looks like: the constant becomes `0xFFFFFFFF80000000`. For a full 64-bit constant, load it into a register first with `mov rcx, 0x8000000000000000` and use that register.

## Counting set bits

The **population count** of a number is how many of its bits are 1. The simple method tests the low bit, then shifts:

```asm
    mov rcx, rdi
    and ecx, 1       # the lowest bit
    add eax, ecx
    shr rdi, 1
```

repeated 64 times. A faster idea, called Kernighan's trick, uses the fact that `x & (x - 1)` **clears the lowest set bit**:

```
x       = 0110 1000
x - 1   = 0110 0111
x&(x-1) = 0110 0000   <- one bit fewer
```

So count how many times you can do that before `x` becomes zero, once per set bit rather than once per position:

```asm
popcount64:
    xor eax, eax            # count = 0
.Lloop:
    test rdi, rdi
    jz .Ldone               # no bits left
    lea rcx, [rdi - 1]      # rcx = x - 1
    and rdi, rcx            # x &= x - 1
    inc eax                 # count++
    jmp .Lloop
.Ldone:
    ret
```

### Why `x & (x - 1)` works

Subtracting 1 from a binary number does a predictable thing: it **borrows** from the lowest set bit. That bit becomes 0, and every zero below it (which had nothing to give) becomes 1. Everything above is unchanged:

```
x       = 0110 1000        lowest set bit is bit 3
x - 1   = 0110 0111        bit 3 cleared, bits 2..0 became 1
x & x-1 = 0110 0000        the bits above bit 3 agree and survive;
                           bit 3 and the low bits disagree, so they become 0
```

So `x & (x - 1)` always removes exactly one 1-bit, the lowest, and never anything else. Repeat until `x` is zero and the number of repeats is the population count. For `x = 0` the loop does not run at all, and for `x = 0xFFFFFFFFFFFFFFFF` it runs 64 times.

Modern x86 CPUs also have a single `popcnt` instruction that does all of this in hardware. It is worth knowing it exists. Here the point is to build the loop yourself: the same "clear the lowest bit" step shows up in bitmap allocators and bit-set iteration, which you will meet in operating systems.

### Tracing a small case

For `x = 0b1101` (13), the loop state is:

| Pass | `rdi` before | `rdi - 1` | `rdi` after | `eax` |
|---|---|---|---|---|
| 1 | `1101` | `1100` | `1100` | 1 |
| 2 | `1100` | `1011` | `1000` | 2 |
| 3 | `1000` | `0111` | `0000` | 3 |
| check | `0000` | | | `jz` taken, return 3 |

## Which registers may you use?

The calling convention splits registers into two groups:

- **Caller-saved** (a function may freely overwrite them): `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8` to `r11`.
- **Callee-saved** (a function must restore them before returning): `rbx`, `rbp`, `r12` to `r15`. The caller is allowed to keep a live value in `rbx` across your call, and if you change it the caller silently breaks.

This lesson only needs caller-saved registers. Our test program fills `rbx`, `rbp` and `r12` to `r15` with magic numbers before calling you and checks them afterwards, so avoid them for now. Later you will learn how to borrow them safely.

## Return values and `eax`

`int` is 32 bits, so the result is read from `eax`. Writing `eax` automatically zeroes the upper half of `rax`, which is why `xor eax, eax` clears the whole register.

## Common mistakes

- Using `sar` on unsigned data: it copies the sign bit, so it never reaches zero for a number with the top bit set.
- Modifying `rdi` and forgetting the argument is gone. That is fine here, just be aware of it.
- Using `rbx` as scratch without saving it. Prefer `rax`, `rcx`, `rdx`, `rsi`, `r8` to `r11`.

Now implement `popcount64`.
