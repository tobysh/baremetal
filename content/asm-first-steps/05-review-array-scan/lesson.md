---
title: "Review: Scanning an Array"
summary: A consolidation exercise. Combine the counted loop, signed comparisons and the clamp pattern in two assembly functions that scan an array. No new instructions.
minutes: 25
difficulty: medium
kind: review
tags: loops, memory, branching
id: asm-first-steps/review-array-scan
mode: harness
---
This is a **review lesson**. It introduces no new instructions. You have now written straight-line code, a branch, and a loop over memory. Here you put those together, because in real assembly they are almost never used alone.

## What you already know

- **Registers and the calling convention:** the first six integer arguments arrive in `rdi rsi rdx rcx r8 r9`, and the result leaves in `rax`.
- **Flags and branches:** `cmp a, b` sets the flags from `a - b`, and the conditional jump you pick says how to read them. `jl`/`jg`/`jle`/`jge` are for **signed** values, `jb`/`ja`/`jbe`/`jae` for **unsigned** ones.
- **A loop over memory:** an index register, `[rdi + rcx*8]` to reach element `i` of an array of 8-byte values, and a `cmp` against `n` at the top so that `n = 0` never touches memory.

## The pieces, side by side

Here is the skeleton of every loop in this course:

```asm
    xor ecx, ecx            # i = 0
.Ltop:
    cmp rcx, rsi
    jae .Ldone              # unsigned: n is a size_t
    ...                     # work on [rdi + rcx*8]
    inc rcx
    jmp .Ltop
.Ldone:
    ret
```

Notice that the index comparison is **unsigned** (`jae`), because `n` is a `size_t`. The element values, on the other hand, are `long`s that may be negative, so anything you do with *them* needs the **signed** jumps. Mixing the two up is the most common bug in this lesson, and a good thing to have felt once.

## Function 1: a clamped sum

`sum_clamped(a, n, lo, hi)` adds up the elements, but forces each one into the range `lo..hi` first. Per element that is exactly the clamp lesson: if it is below `lo`, use `lo`; if it is above `hi`, use `hi`; otherwise keep it. Then add the result to the total. You need a scratch register for the current element, one for the index, and one for the total (`rax`).

The arguments use four registers this time: `a` in `rdi`, `n` in `rsi`, `lo` in `rdx`, `hi` in `rcx`. That means `rcx` is **not** free for your loop counter, which is a difference from the loop lesson. Pick `r8` or `r9` for the index instead. All of `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`-`r11` are yours to overwrite; you do not need to save them.

## Function 2: counting negatives

`count_negative(a, n)` returns how many elements are below zero. Compare each element with 0 using a signed condition and add 1 to the counter when it holds. `cmp qword ptr [rdi + rcx*8], 0` compares memory against an immediate directly, and `qword ptr` says how many bytes to read, since `0` alone does not tell the assembler.

Remember: labels such as `.Lloop` must be unique across the whole file, so give the second function its own names.
