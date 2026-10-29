---
title: Sign and Zero Extension
summary: Bytes and 32-bit ints must be widened before you add them to a 64-bit total. movzx widens unsigned values, movsx and movsxd widen signed ones.
minutes: 20
difficulty: easy
tags: instructions, arrays, signed-unsigned
id: asm-intermediate/sign-and-zero-extend
mode: harness
---
Arrays of `char` or `int` are read one element at a time, but sums usually live in 64-bit registers. Loading a byte into a wider register needs a decision: are the upper bits **zeros** or a copy of the **sign bit**?

| Instruction | Meaning | Example |
|---|---|---|
| `movzx rdx, byte ptr [rdi]` | zero-extend: `0xFF` becomes 255 | `uint8_t` |
| `movsx rdx, byte ptr [rdi]` | sign-extend: `0xFF` becomes -1 | `int8_t` |
| `movsxd rdx, dword ptr [rdi]` | sign-extend 32 to 64 bits | `int32_t` |
| `mov edx, dword ptr [rdi]` | writing `edx` zeroes the upper half of `rdx` | `uint32_t` |

The wrong choice gives a plausible but wrong total, and the bug only appears when a byte has its top bit set. A byte array `{200, 100}` summed with `movsx` gives `-56 + 100 = 44`, not 300.

## The loop

```asm
sum_i8:
    xor eax, eax               # total
    xor ecx, ecx               # index
.Lloop:
    cmp rcx, rsi
    jae .Ldone
    movsx rdx, byte ptr [rdi + rcx]
    add rax, rdx
    inc rcx
    jmp .Lloop
.Ldone:
    ret
```

The element size is 1, so the index needs no scale. For 4-byte elements write `[rdi + rcx*4]`.
