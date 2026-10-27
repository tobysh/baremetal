---
title: Bit Twiddling
summary: Shift, mask and clear bits with and, shr and sub to count the set bits in a 64-bit number, and learn which registers you must preserve.
minutes: 25
difficulty: medium
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
