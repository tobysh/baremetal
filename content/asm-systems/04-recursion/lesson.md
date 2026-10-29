---
title: Recursion
summary: A function that calls itself needs its own copy of every value it still needs. Write recursive factorial and Fibonacci and watch the stack grow and shrink.
minutes: 30
difficulty: medium
tags: recursion, stack, calling-convention
id: asm-systems/recursion
mode: harness
---
Recursion needs no special instruction. `call fact` inside `fact` is an ordinary call. What makes it work is that **each activation keeps its own state on the stack**, because registers are shared by all activations and the recursive call will overwrite them.

## Factorial

```c
unsigned long fact_rec(unsigned long n) {
    if (n <= 1) return 1;
    return n * fact_rec(n - 1);
}
```

After the recursive call returns you still need the original `n`. Save it on the stack:

```asm
fact_rec:
    cmp rdi, 1
    ja  .Lrecurse
    mov eax, 1              # base case
    ret
.Lrecurse:
    push rdi                # save n (also fixes alignment: 8 -> 0 mod 16)
    dec rdi                 # n - 1
    call fact_rec           # rax = fact(n-1)
    pop rdi                 # n again
    imul rax, rdi
    ret
```

A single `push` neatly restores the 16-byte alignment, because on entry `rsp` is 8 mod 16. Use `ja` and `jb` (unsigned) because the arguments are `unsigned long`.

## Fibonacci: two recursive calls

```c
unsigned long fib_rec(unsigned long n) {
    if (n < 2) return n;
    return fib_rec(n - 1) + fib_rec(n - 2);
}
```

Now you have to hold *two* things across calls: `n` (needed for the second call) and the result of the first call (needed for the final addition). A neat approach is to reserve scratch slots and keep alignment in one step: `sub rsp, 24` on entry makes `rsp` 0 mod 16 and leaves room for two 8-byte slots at `[rsp]` and `[rsp + 8]`. Release them with `add rsp, 24` before `ret`.

## How deep can it go?

Each activation costs at least its return address (8 bytes) plus whatever it pushed. The default stack is 8 MiB, so recursion a few hundred thousand levels deep overflows it. Naive Fibonacci is also exponential: `fib_rec(30)` makes over 2.6 million calls. Real code would loop instead, but the shape of this function is the one you need whenever a recursion has two branches (trees, divide and conquer, and parsers).

## Debug tip

Draw the stack for `fact_rec(3)`. It holds return address, saved 3, return address, saved 2, return address, saved 1 (never pushed: it hits the base case), then unwinds multiplying. If your stack picture has a different number of pushes than pops on any path, the function will return to a wrong address.
