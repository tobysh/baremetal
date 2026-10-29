---
title: Callee-Saved Registers and Calling Out
summary: A callee may trash most registers. Learn which ones survive, how to save the rest, and why the stack must be 16-byte aligned at every call.
minutes: 30
difficulty: medium
tags: calling-convention, stack, registers
id: asm-systems/callee-saved
mode: harness
---
A leaf function (one that calls nothing) can use `rax`, `rcx`, `rdx`, `rsi`, `rdi` and `r8`-`r11` as it likes. The moment you `call` something else, you have a problem: that function is allowed to overwrite exactly those registers. Anything you need after the call must live somewhere safe.

## Two safe places

1. **Callee-saved registers**: `rbx`, `rbp`, `r12`, `r13`, `r14`, `r15`. A function that wants to use one must save its old value on entry and restore it before `ret`. In exchange, every function you call promises to give them back unchanged.
2. **Your own stack slots**, addressed from `rsp`.

The standard pattern for the first option:

```asm
f:
    push rbx            # save the caller's rbx
    push r12
    mov  rbx, rdi       # now rbx is mine: it survives calls
    ...
    pop  r12            # restore in reverse order
    pop  rbx
    ret
```

Push and pop must match exactly, in reverse order, on every path that reaches `ret`.

## The alignment rule

The ABI promises that **`rsp` is a multiple of 16 immediately before every `call` instruction**. Because `call` pushes an 8-byte return address, `rsp` is therefore `16n + 8` at the first instruction of every function. Each `push` changes this by 8:

| State | `rsp` mod 16 |
|---|---|
| on entry | 8 |
| after 1 push | 0 (ready to call) |
| after 2 pushes | 8 (misaligned!) |
| after 5 pushes | 0 (ready to call) |

So a function that pushes an even number of registers must also `sub rsp, 8` (and `add rsp, 8` later). Misalignment often works for years and then crashes inside `printf` or `memcpy` on an SSE instruction that demands aligned memory. The hidden test below checks the alignment at every call you make.

## Calling through a pointer

A function pointer is called with `call reg`:

```asm
mov rdi, [rbx + r13*8]    # the argument
call r15                  # call the function whose address is in r15
```

The result comes back in `rax`. This is how `qsort` calls your comparison function, and how interpreters call builtins.

## The plan for this lesson

```c
size_t count_matching(const long *a, size_t n, long (*pred)(long)) {
    size_t count = 0;
    for (size_t i = 0; i < n; i++)
        if (pred(a[i])) count++;
    return count;
}
```

You need four values to survive every call to `pred`: the array pointer, `n`, the index and the count, plus the pointer `pred` itself: five values, so five callee-saved registers (`rbx`, `r12`-`r15`). Five pushes also happens to leave the stack aligned. The hidden `pred` deliberately scrambles every caller-saved register and reports it if you call it with a misaligned stack, so keeping things in `rcx` or `r8` will fail.
