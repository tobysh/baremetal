---
title: The rep String Instructions
summary: stos, movs and scas work on strings of bytes with rdi and rsi as cursors, repeated by rep. Direction decides whether copies can overlap safely.
minutes: 40
difficulty: hard
tags: strings, memory, instructions
id: asm-intermediate/string-instructions
mode: harness
---
x86 has instructions built for `memset`, `memcpy` and `strlen`. They use **fixed registers**:

| Instruction | Does | Registers |
|---|---|---|
| `stosb` | `[rdi] = al`, then step `rdi` | `al`, `rdi` |
| `movsb` | `[rdi] = [rsi]`, then step both | `rdi`, `rsi` |
| `scasb` | compare `al` with `[rdi]`, then step `rdi` | `al`, `rdi` |

`rep` repeats the instruction `rcx` times. `repne` repeats while the compared bytes differ, which is how you search for a byte.

```asm
mov rcx, rdx            # count
mov eax, esi            # byte to store
rep stosb               # memset(rdi, al, rcx)
```

There are `w`, `d` and `q` forms (`stosq` stores `rax`, 8 bytes at a time) too.

## The direction flag

Each step moves the cursors **forward** when the direction flag DF is 0 (`cld`) and **backward** when DF is 1 (`std`). The ABI promises that DF is clear on entry **and on exit** of every function, so if you `std`, you must `cld` before `ret`.

Backward copying matters for overlapping buffers. `memmove(dst, src, n)` where `dst` is higher than `src` and the ranges overlap must copy from the end, or the source gets overwritten before it is read. Start both cursors at the last byte: `lea rdi, [rdi + rcx - 1]`.

## Counting with repne scasb

To measure a string: `xor eax, eax` (search for 0), `mov rcx, -1` (an effectively infinite count) and `repne scasb`. Afterwards `rcx` has been decremented once per byte examined, **including** the NUL, so the length is `~rcx - 1`.

`rep movsb` is surprisingly fast on modern CPUs; libc uses it for large copies.
