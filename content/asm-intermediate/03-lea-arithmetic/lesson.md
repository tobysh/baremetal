---
title: lea as a Calculator
summary: lea computes base + index*scale + offset without touching memory or flags. Compilers use it for multiplication by small constants.
minutes: 20
difficulty: easy
tags: instructions, arithmetic, pointers
id: asm-intermediate/lea-arithmetic
mode: harness
---
`lea` ("load effective address") evaluates a memory operand **but does not read memory**. It just stores the address arithmetic:

```asm
lea rax, [rdi + rsi*8 + 16]     # rax = rdi + rsi*8 + 16
```

The scale may be 1, 2, 4 or 8. It takes three inputs, writes a separate destination, and leaves the flags alone. That makes it a free three-operand add-and-shift, and a multiplier for constants:

```asm
lea rax, [rdi + rdi*4]          # rax = x * 5
add rax, rax                    # rax = x * 10
```

`x*9` is `lea rax, [rdi + rdi*8]`. A constant like 85 is `5 * 17`: multiply by 5 with `lea`, then by 17 with a shift and an add (`x*16 + x`).

Pointer arithmetic is the same idea: `&p[i]` for an array of `long` is `lea rax, [rdi + rsi*8]`.

This is what `gcc -O2` emits for `x * 10`. `imul rax, rdi, 10` also works, but `lea` is cheaper on most CPUs and is the idiom you will see in disassembly.
