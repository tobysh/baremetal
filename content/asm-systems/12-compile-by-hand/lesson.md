---
title: Compiling by Hand: Structs
summary: Translate C functions that use a struct into assembly the way a compiler does. Work out field offsets, padding and sign extension from the type layout.
minutes: 40
difficulty: medium
tags: compiler, structs, memory
id: asm-systems/compile-by-hand
mode: harness
---
A compiler is mostly a careful, mechanical translator. You can learn its habits by doing the same job on paper: read the C, decide the data layout, assign registers, and emit instructions. The skill pays off when you read real compiler output, which is the next lesson.

## Step 1: work out the layout

```c
struct item {
    int  weight;   // offset 0, 4 bytes
    int  bonus;    // offset 4, 4 bytes
    long value;    // offset 8, 8 bytes (must start on an 8-byte boundary)
};                 // sizeof = 16
```

Rules: every field sits at an offset that is a multiple of its own size ("alignment"), and the struct's total size is rounded up to a multiple of its largest alignment. Here there is no padding. If `bonus` were a `char`, the compiler would insert 3 bytes of padding after it so `value` stays on a multiple of 8 — you can check any layout with `offsetof` and `sizeof`.

So `p->weight` is `[rdi]`, `p->bonus` is `[rdi + 4]`, `p->value` is `[rdi + 8]`, and the next element of an array of items is `rdi + 16`.

## Step 2: types decide the instruction

`weight` and `bonus` are `int`s, so loads must sign-extend to 64 bits when mixed with `long` (`movsxd`). The multiplication `(long)p->weight * p->value` is a 64-bit `imul`. A comparison `p->weight > 0` on an `int` is a **signed 32-bit** compare: `test edx, edx` then `jle`/`jg`.

## Step 3: registers and control flow

Write the C as a loop with explicit pieces (initialise, test, body, step) and give each variable a register. Then each `if` becomes a conditional jump that **skips** the body when the C condition is false: invert the condition.

```c
if (p->weight > 0) { sum += ...; }     // jle .Lskip  ... body ... .Lskip:
```

## Check your work against gcc

You can ask the real compiler for its opinion on your machine:

```text
gcc -O1 -S -masm=intel -fno-asynchronous-unwind-tables -o - file.c
```

Compare structure, not exact instructions. `-O0` output stores every variable in memory and is easy to read; `-O1` or `-O2` keeps them in registers, is shorter and looks like what you are about to write. Compilers prefer `xor eax, eax` to `mov eax, 0`, `lea` for arithmetic, and `test r, r` to `cmp r, 0`.

## This lesson's functions

```c
struct item { int weight; int bonus; long value; };

long total_score(const struct item *p, size_t n) {
    long sum = 0;
    for (size_t i = 0; i < n; i++, p++)
        if (p->weight > 0)
            sum += (long)p->weight * p->value + p->bonus;
    return sum;
}

long heaviest(const struct item *p, size_t n) {   // index of the first item with the largest weight, -1 if n == 0
    if (n == 0) return -1;
    size_t best = 0;
    for (size_t i = 1; i < n; i++)
        if (p[i].weight > p[best].weight) best = i;
    return best;
}
```

Items with zero or negative weight are skipped by `total_score`. In `heaviest`, an equal weight does not replace the current best, so ties pick the first.
