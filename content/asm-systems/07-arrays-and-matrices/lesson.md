---
title: Arrays, Strides and Pointer Arithmetic
summary: Walk memory with scaled indexes and with pointers that advance by a stride. Handle 4-byte elements with sign extension and a 2D matrix stored row by row.
minutes: 30
difficulty: medium
tags: memory, arrays, pointers
id: asm-systems/arrays-and-matrices
mode: harness
---
In C, `a[i]` means `*(a + i)`, and the compiler multiplies `i` by the element size. In assembly you do that yourself, or let the addressing mode do it.

## Scaled index versus advancing pointer

For 8-byte elements, `[rdi + rcx*8]` reads element `rcx`. The scale factor can only be 1, 2, 4 or 8, matching the common element sizes. The alternative is to move the pointer itself:

```asm
add rdi, 8          # next long
```

The pointer style frees a register (no index) and is how compilers write pointer loops. The index style is easier to reason about. Use whichever gives you the simpler loop condition.

## 4-byte elements and sign extension

A C `int` is 32 bits. Reading it with `mov eax, [rdi]` loads 32 bits and zero-fills the top half of `rax`. For a negative `int` that is wrong if you want a 64-bit `long`. Use **`movsxd`**, which sign-extends:

```asm
movsxd rdx, dword ptr [rdi]     # rdx = (long)*p
```

Remember that writing to a 32-bit register (`mov eax, ...`, `xor ecx, ecx`) always zeroes the upper 32 bits, while writing 8-bit or 16-bit registers does not.

## In-place update: prefix sums

`prefix_sums(a, n)` replaces each element by the sum of all elements up to and including it: `{3, 1, 4, 1}` becomes `{3, 4, 8, 9}`. Keep a running total in a register: load, add, store back. One pass, no second array.

## Matrices: row-major layout

An `n` by `n` matrix of `int` is stored as `n*n` consecutive ints, row after row. Element `(r, c)` lives at index `r*n + c`, so at byte offset `(r*n + c)*4`. The diagonal elements `(0,0), (1,1), ...` are therefore `n + 1` ints apart:

```text
index:  0  1  2 | 3  4  5 | 6  7  8
        *        |    *     |       *     stride = n + 1 = 4 ints = 16 bytes
```

`trace(m, n)` sums them: start the pointer at `m`, add `(n + 1) * 4` bytes after each element, and stop after `n` elements. Compute that byte stride once with `lea r8, [rsi + 1]` and `shl r8, 2`.

Mind the empty case: for `n = 0` nothing must be read.
