---
title: The Internet Checksum
summary: Sum 16-bit words, fold the carries back in and invert. The checksum of IP, TCP and UDP is a tidy exercise in movzx, folding and odd-length tails.
minutes: 35
difficulty: medium
tags: networking, bit-manipulation, loops
id: asm-intermediate/internet-checksum
mode: harness
---
IPv4, TCP and UDP protect their headers with the **one's-complement checksum** of RFC 1071:

1. Add up the data as 16-bit words.
2. If the sum exceeds 16 bits, add the overflow (the "carry") back into the low 16 bits, repeating until nothing is left above bit 15.
3. Return the bitwise NOT, truncated to 16 bits.

A receiver runs the same sum over the data *including* the stored checksum and gets `0xFFFF` before the NOT, i.e. a checksum of **0**. That is how corruption is detected.

## Odd lengths

If the length is odd, the last byte is treated as a word whose other byte is zero (the byte in the *low* half on a little-endian CPU, which is how this lesson reads words).

## Plan

Accumulate in a 64-bit register so no partial carries are lost, and fold once at the end:

```asm
movzx edx, word ptr [rdi]      # one 16-bit word, zero-extended
add rax, rdx
```

Fold with a loop: while `rax >> 16` is non-zero, `rax = (rax & 0xFFFF) + (rax >> 16)`:

```asm
.Lfold:
    mov rdx, rax
    shr rdx, 16
    jz  .Ldone              # shr sets ZF when the result is 0
    movzx eax, ax
    add rax, rdx
    jmp .Lfold
```

Words are read in memory (little-endian) order. The one's-complement sum is the same whichever byte order you use, up to a byte swap of the final result, so the value you return can be stored straight back into the packet's checksum field with no extra swap.
