---
title: "Review: Recursion Meets libc"
summary: A consolidation exercise. Combine a recursive call with a libc search loop whose state lives in callee-saved registers. No new instructions.
minutes: 25
difficulty: medium
kind: review
tags: recursion, libc, calling-convention
id: asm-systems/review-gcd-and-count
mode: harness
---
This is a **review lesson**. It introduces nothing new. You have written a recursive function that saves what it needs across its own call, and a function that calls libc. Here is one of each, with a small twist.

## `gcd_rec`

Euclid's algorithm in C:

```c
unsigned long gcd_rec(unsigned long a, unsigned long b) {
    if (b == 0) return a;
    return gcd_rec(b, a % b);
}
```

The twist is that nothing needs to be saved across the call: it is a **tail call**, so there is no work after the recursion. You can write it as a call with the usual alignment care, or turn it into a `jmp` back to the top. The remainder comes from `div`: `xor edx, edx` / `mov rax, rdi` / `div rsi` leaves `a % b` in `rdx`.

## `count_char`

`strchr(s, c)` returns a pointer to the first occurrence of `c` in `s`, or `NULL`. Counting every occurrence is a loop that restarts the search one byte after each hit. The loop state (the current position and the running count, and also `c`) must survive the libc call, so use callee-saved registers and keep the stack aligned: three pushes do both.

Two cautions. Searching for the NUL character (`c = 0`) finds the terminator itself every time, so the function must return 0 for it rather than loop forever. And `c` is an `int` whose useful value is in `esi`.
