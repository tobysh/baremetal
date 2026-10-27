---
title: Writing Through Pointers
summary: Modify an array in place with loads and stores, using two pointers that walk toward each other from both ends.
minutes: 25
difficulty: medium
id: asm-first-steps/reverse-array
mode: harness
---
Last time you only *read* memory. Now you will change it, which is how assembly functions like `memcpy`, `qsort` and `strcpy` do their work.

## Loads and stores

A **load** copies memory into a register, a **store** copies a register into memory. x86 does not allow a memory-to-memory `mov`, so moving a value from one place in memory to another needs a register in between:

```asm
mov rax, [rdi]       # load  a[0]
mov rcx, [rsi]       # load  b[0]
mov [rdi], rcx       # store into a[0]
mov [rsi], rax       # store into b[0]
```

That is exactly a **swap**: load both values first, then store each into the other's slot. Loading both *before* storing is what keeps either from being lost.

## Two pointers

To reverse an array you swap the first element with the last, the second with the second-to-last, and so on until the pointers meet in the middle. Keep one pointer at the start and one at the end:

```c
long *lo = a, *hi = a + n - 1;
while (lo < hi) { swap(*lo, *hi); lo++; hi--; }
```

In assembly, a pointer is a register, and moving it by one `long` means adding 8, because pointer arithmetic counts bytes:

```asm
add rdi, 8        # lo++
sub rsi, 8        # hi--
```

## Tracing it by hand

Take `a = {1, 2, 3, 4, 5}` and `n = 5`. Write `lo` and `hi` as element positions, even though in the registers they are byte addresses (`a` plus 0, 8, 16...):

| Check | `lo` | `hi` | Array | Decision |
|---|---|---|---|---|
| 1 | 0 | 4 | `1 2 3 4 5` | `0 < 4`: swap positions 0 and 4, then move inward |
| 2 | 1 | 3 | `5 2 3 4 1` | `1 < 3`: swap positions 1 and 3, then move inward |
| 3 | 2 | 2 | `5 4 3 2 1` | `2 >= 2`: stop |

The middle element `3` is never touched, because the pointers meet on it. With an even length such as `{1,2,3,4}` the pointers never meet: after two swaps `lo = 2` and `hi = 1`, so they have *crossed*. That is why the stop test is `lo >= hi` and not `lo == hi`, which would run off the end of the array.

## Computing the end pointer

The address of the last element is `a + (n-1)*8`. A single `lea` does the multiplication and the add:

```asm
lea rsi, [rdi + rsi*8 - 8]     # hi = a + n*8 - 8
```

Here `rsi` held `n`, and is overwritten by the pointer. Reusing argument registers like this is fine: in this calling convention they are *caller-saved*, so you may clobber them freely (more on that in the next lessons).

## Comparing pointers

Addresses are unsigned numbers, so use `jae` / `jb`:

```asm
.Lloop:
    cmp rdi, rsi
    jae .Ldone         # stop when lo >= hi
```

This also handles odd lengths: the middle element is left alone because the pointers become equal.

## Edge cases

- `n = 0`: there is no last element. Return before computing `a - 8`.
- `n = 1`: nothing to swap; the loop condition `lo >= hi` is true right away.

## Why this matters outside exercises

The load-then-store shape of the swap is the pattern behind nearly every memory routine. `memcpy` is a loop of one load and one store. `strlen` is a loop of loads. `qsort` is made of compares and swaps like this one. Once you can write these loops in assembly, you can read what a compiler produced for the C versions, and you will see that it is almost always the same handful of ideas: a pointer or index in a register, a load, a compare, a store, a jump back.

Two habits make these loops easy to get right:

1. **Say what each register holds** before writing any code (a comment table like the one in the next lesson helps). Most bugs are one register used for two jobs.
2. **Check the loop on `n = 0`, `1`, `2` and `3`**. Those four sizes cover empty, trivial, the smallest even case and the smallest odd case, which is where almost every off-by-one lives.

## Common mistakes

- Overwriting `[rdi]` before you have loaded its old value.
- Adding 1 instead of 8 to a pointer.
- Looping while `lo != hi`, which never stops for even lengths because the pointers cross.
- Writing outside the array (an off-by-one on the end pointer).

Now implement `reverse_array`.
