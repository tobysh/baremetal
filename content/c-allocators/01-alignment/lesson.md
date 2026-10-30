---
title: Alignment Arithmetic
summary: Round sizes and addresses up to a power-of-two boundary with bit tricks, avoid overflow, and rebuild the compiler's struct layout by hand.
minutes: 25
difficulty: easy
tags: alignment, bit-tricks, struct-layout
id: c-allocators/alignment
mode: harness
---
Every allocator answers one question over and over: *where may this object start?* CPUs prefer, and sometimes require, that a 4-byte `int` starts at an address divisible by 4, an 8-byte `double` at one divisible by 8, and so on. That rule is **alignment**. An address `p` is aligned to `a` when `p % a == 0`.

## Rounding up with a mask

Alignments in C are powers of two (1, 2, 4, 8, 16 ...). A power of two has a single bit set, so `a - 1` is a mask of all the lower bits:

```text
a     = 16 = 0b10000
a - 1 = 15 = 0b01111
```

An address is a multiple of `a` exactly when those low bits are zero. To round `n` **up** to the next multiple, add `a - 1` and clear the low bits:

```c
(n + (a - 1)) & ~(a - 1)
```

Rounding **down** is just `n & ~(a - 1)`. Check it: `align_up(13, 8)` is `(13 + 7) & ~7 = 20 & ~7 = 16`.

## Traps

- **Not a power of two.** The trick silently gives garbage for `a = 12`. Test with `(a & (a - 1)) == 0` and `a != 0`.
- **Overflow.** `n + (a - 1)` wraps around for huge `n`, and the result looks like a small valid size. An allocator that does this hands out too little memory. Detect it before it happens.
- **Pointers.** To align an address, convert it to `uintptr_t`, round, and convert back. The padding you skip is `align_up(addr, a) - addr`.

## Struct layout is the same arithmetic

The compiler places each member at the next offset that is a multiple of its alignment, then pads the struct's total size up to a multiple of the largest member alignment:

```c
struct S { char c; int i; char d; double x; };
// c at 0, i at 4 (3 bytes padding), d at 8, x at 16 (7 bytes padding), size 24
```

The tests use the compiler itself (`offsetof`, `sizeof`) as the reference for your layout function.

## Your turn

Implement the five functions in the starter. The task lists their exact behaviour.
