---
title: Insertion Sort
summary: Sort an array in place with nested loops, shifting elements right to open a gap. Practice scaled addressing with two live indices.
minutes: 35
difficulty: medium
tags: algorithms, arrays, loops
id: asm-intermediate/insertion-sort
mode: harness
---
Insertion sort keeps a sorted prefix and grows it one element at a time. For element `i`, lift it out as `key`, shift every larger element of the prefix one slot to the right, and drop `key` in the gap.

```c
void isort(long *a, size_t n) {
    for (size_t i = 1; i < n; i++) {
        long key = a[i];
        size_t j = i;
        while (j > 0 && a[j - 1] > key) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = key;
    }
}
```

## Register plan

With no calls there is no need for the stack: `rdi` = array, `rsi` = n, `r8` = `i`, `rcx` = `j`, `rax` = `key`, `rdx` = a scratch element.

The addressing mode does the multiplication by 8: `a[j - 1]` is `[rdi + rcx*8 - 8]`, a single operand.

## Comparing signed values

The array holds signed `long`s, so use the signed jumps: `jle`, `jg`, `jl`. Using `jbe` here is the classic unsigned-comparison bug and only shows up when the data has negative numbers.

## Counting inversions

An **inversion** is a pair `i < j` with `a[i] > a[j]`. A sorted array has none; a reversed array of `n` elements has `n(n-1)/2`. Insertion sort does one shift per inversion. Counting them takes two nested loops; `setg cl` turns each comparison into a 0 or 1 you can add straight to the total.
