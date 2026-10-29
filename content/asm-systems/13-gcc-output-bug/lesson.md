---
title: Reading gcc Output and Finding the Bug
summary: Read unoptimised compiler output, map every instruction back to the C source, and fix three bugs that were planted in it.
minutes: 35
difficulty: hard
tags: compiler, debugging, signed-unsigned
id: asm-systems/gcc-output-bug
mode: harness
---
Most assembly you will ever meet was produced by a compiler, and most of the time you read it to find out why something went wrong. This lesson gives you a function that is **almost** what `gcc -O0 -masm=intel` generates for the C below, with three mistakes introduced. Your job is to find and fix them without rewriting the function.

```c
int count_above(const int *a, int n, int limit) {
    int count = 0;
    for (int i = 0; i < n; i++)
        if (a[i] > limit)
            count++;
    return count;
}
```

## How to read `-O0` output

Unoptimised code keeps every variable in memory at a fixed offset from `rbp` and reloads it for each use. That makes the mapping from C to assembly almost line by line:

| C variable | Location |
|---|---|
| `a` (argument, rdi) | `[rbp - 24]` |
| `n` (esi) | `[rbp - 28]` |
| `limit` (edx) | `[rbp - 32]` |
| `count` | `[rbp - 4]` |
| `i` | `[rbp - 8]` |

The prologue spills the register arguments to those slots; the epilogue loads the return value into `eax` and unwinds the frame. Note that here the function does not call anything, so there is no `sub rsp`: it uses the red zone below `rsp`.

A `for` loop is laid out with the condition at the **bottom**: the code jumps to the test first, and the test jumps back up to the body while it holds.

```asm
    jmp .Ltest
.Lbody:
    ...
    add dword ptr [rbp - 8], 1      # i++
.Ltest:
    mov eax, [rbp - 8]
    cmp eax, [rbp - 28]             # i - n
    jl  .Lbody                      # continue while i < n
```

## Which jump?

After `cmp x, y` the signed conditions are `jl jle jg jge`; the unsigned ones are `jb jbe ja jae`. The C types decide: `int` comparisons use the signed jumps, `size_t` and pointers the unsigned ones. Mixing them up works for small positive numbers and fails for negatives, which is exactly the kind of bug that survives a first round of tests.

The other classic sources of error: an off-by-one in the loop condition (`jle` where `jl` belongs reads one element past the array), and returning or storing the wrong stack slot.

## How to approach it

1. Annotate each line of the given code with the C it implements.
2. Walk through a tiny example by hand, with `n = 2`, to see whether the loop stops at the right time.
3. Try a negative element and a negative `limit`.
4. Check what is loaded into `eax` right before `ret`.

Work out the three bugs yourself; the hidden tests exercise negative numbers, the array bounds and the final result separately.
