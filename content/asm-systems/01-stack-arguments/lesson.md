---
title: Arguments Beyond Six
summary: Registers carry only six integer arguments. Learn where the seventh and eighth live, and why the stack pointer is the key to finding them.
minutes: 20
difficulty: medium
tags: calling-convention, stack
id: asm-systems/stack-arguments
mode: harness
---
In the first track every function took three arguments or fewer. The **System V AMD64** calling convention passes the first six integer or pointer arguments in registers, in this fixed order:

| Argument | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|
| Register | `rdi` | `rsi` | `rdx` | `rcx` | `r8` | `r9` |

Floating-point arguments use `xmm0`-`xmm7` separately (you will meet those later). Everything that does not fit goes **on the stack**, pushed right to left, so the seventh argument is closest to the stack pointer.

## What the stack looks like on entry

When the caller executes `call f`, the CPU pushes the return address. So at the very first instruction of `f`:

```text
higher addresses
  [rsp + 24]   ... caller's own data
  [rsp + 16]   8th argument
  [rsp + 8]    7th argument
  [rsp]        return address   <-- rsp
lower addresses
```

The seventh argument is therefore `[rsp + 8]`, not `[rsp]`. Forgetting the return address is the classic off-by-one-slot mistake. Read them like any other memory operand:

```asm
mov r10, [rsp + 8]      # 7th argument
add rax, [rsp + 16]     # add the 8th directly from memory
```

If you later `push` anything, `rsp` moves and these offsets shift by 8 for each push. Read stack arguments early, or remember to adjust.

## Which registers may you use freely?

The convention splits the registers in two:

- **Caller-saved** (the function may overwrite them): `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`-`r11`.
- **Callee-saved** (the function must restore them before `ret`): `rbx`, `rbp`, `r12`-`r15`.

You can do all the work in this lesson with caller-saved registers (`r10` and `r11` are free scratch registers that are not arguments). The hidden test checks that `rbx`, `rbp` and `r12`-`r15` come back unchanged.

## Useful arithmetic shortcuts

`lea` computes an address without touching memory, which makes it a free multiply-and-add: `lea rax, [rax + rsi*2]` adds `2*rsi` to `rax`. The scale can be 1, 2, 4 or 8. For other factors use `imul r10, r10, 7` (destination, source, immediate).

Now write `weighted8`, which gives every argument a different weight so that mixing them up cannot pass by accident.
