---
title: Detecting Overflow
summary: The overflow flag marks signed wraparound, the carry flag unsigned wraparound. Use jo, jc and mul to write arithmetic that refuses to wrap.
minutes: 30
difficulty: medium
tags: flags, arithmetic, signed-unsigned
id: asm-intermediate/overflow-detection
mode: harness
---
The CPU has one adder for signed and unsigned numbers; it reports both kinds of overflow separately.

| Flag | Meaning after `add`/`sub` | Jumps |
|---|---|---|
| **CF** | unsigned result does not fit (carry or borrow) | `jc`/`jb`, `jnc`/`jae` |
| **OF** | signed result does not fit | `jo`, `jno` |

`0x7FFF...FFFF + 1` sets OF (signed wrap) but not CF. `0xFFFF...FFFF + 1` sets CF but not OF. Which one you test must match the type you are working with.

```asm
add_checked:
    mov rax, rdi
    add rax, rsi
    jo  .Lfail              # signed overflow
    mov [rdx], rax
    mov eax, 1
    ret
.Lfail:
    xor eax, eax
    ret
```

## Multiplication

`mul src` computes the full 128-bit product `rax * src` in `rdx:rax`. It sets **CF and OF both** when the upper half `rdx` is non-zero, which is exactly "the unsigned product did not fit in 64 bits". So a checked multiply is `mul` followed by `jo` (or `jc`).

Careful: `mul` overwrites `rdx`, so save any pointer arriving there first.

## Writing through the result pointer only on success

The contract in this lesson: the output is written only if the operation succeeded. A caller who sees 0 must find its variable unchanged.
