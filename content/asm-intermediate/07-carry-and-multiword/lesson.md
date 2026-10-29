---
title: The Carry Flag and Multi-Word Arithmetic
summary: add sets the carry flag when the result does not fit; adc feeds it into the next addition. Chain them to add numbers wider than 64 bits.
minutes: 35
difficulty: medium
tags: flags, arithmetic, loops
id: asm-intermediate/carry-and-multiword
mode: harness
---
A 64-bit `add` throws away the 65th bit, but not silently: the **carry flag (CF)** records it. `adc dst, src` ("add with carry") computes `dst + src + CF`. Two instructions add a 128-bit number stored as two 64-bit "limbs":

```asm
add rax, rcx            # low halves, CF = carry out
adc rdx, rbx            # high halves plus that carry
```

Numbers are stored **little-endian by limb**: `a[0]` is the low 64 bits, `a[1]` the next. Memory operands work too: `adc rcx, [rsi + 8]`.

## Loops that must keep CF alive

Almost every arithmetic instruction rewrites CF, including `cmp`, `add` and `inc`'s neighbours. In a loop that chains carries you may use only instructions that **leave CF alone**:

- `mov`, `lea` and `jcc` never change flags,
- `inc` and `dec` change every flag *except* CF.

So the loop counter must be handled with `dec` (or `lea` for the index), not `add`/`cmp`. Clear CF before the first `adc` with `clc` (or `xor`/`test`).

```asm
.Lloop:
    mov  rax, [rdi + r8*8]
    adc  rax, [rsi + r8*8]
    mov  [rdx + r8*8], rax
    lea  r8, [r8 + 1]        # lea: no flags
    dec  rcx                 # dec: CF untouched
    jnz  .Lloop
```

After the loop, `setc al` (or `sbb eax, eax` / `neg eax`) turns the final carry into a return value.

## The high half of a product

`mul src` multiplies `rax` by `src` and writes the 128-bit result to `rdx:rax` (unsigned). The high 64 bits, the part C's `*` discards, are in `rdx`.
