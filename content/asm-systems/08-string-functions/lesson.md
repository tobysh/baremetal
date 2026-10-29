---
title: strlen, memcpy and strcmp
summary: Reimplement three libc staples. Learn movzx byte loads, the rep movsb string instruction, and unsigned byte comparison that never reads past the NUL.
minutes: 35
difficulty: medium
tags: strings, memory, loops
id: asm-systems/string-functions
mode: harness
---
C strings are just bytes ending in `0`. The libc functions you use daily are short loops, and writing them is the best way to learn byte-sized addressing.

## Byte operands

A byte is accessed through an 8-bit register (`al`, `cl`, `dl`) with `byte ptr` when the size is not implied:

```asm
cmp byte ptr [rdi + rax], 0     # is s[i] the terminator?
mov dl, [rsi]                   # load one byte
```

To use a byte in wider arithmetic, **zero-extend** with `movzx` (for `unsigned char`) or sign-extend with `movsx` (for `signed char`). Loading into `al` alone leaves the rest of `rax` as it was, a common source of bugs.

```asm
movzx eax, byte ptr [rdi]       # eax = (unsigned)s[0]
```

## strlen

```c
size_t my_strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
```

The loop must stop **at** the terminator and never read beyond it. The hidden test places the string right in front of an inaccessible page, so one byte too many crashes the program.

## memcpy with `rep movsb`

x86 has dedicated string instructions. `movsb` copies one byte from `[rsi]` to `[rdi]` and advances both; the `rep` prefix repeats it `rcx` times:

```asm
mov rax, rdi        # memcpy returns dst
mov rcx, rdx        # count
rep movsb
ret
```

It relies on the **direction flag** being clear, which the ABI guarantees at function entry. Modern CPUs run `rep movsb` very quickly for large blocks. `rcx = 0` copies nothing.

## strcmp

```c
int my_strcmp(const char *a, const char *b);   // <0, 0, >0
```

C defines the comparison on `unsigned char` values, so a byte `0xC3` is larger than `'A'`. Load both bytes with `movzx`, compare, and stop at the first difference **or** when both reached the terminator. The result may be the difference of the two bytes (`sub eax, ecx`). If the bytes are equal and zero, return 0.

The order of checks matters: compare the two bytes first; if they differ you are done (even if one is the terminator). If they are equal and zero, return 0. Only if they are equal and non-zero do you advance.

All three functions are leaf functions that need only caller-saved registers.
