---
title: Pointer Arithmetic and Array Decay
summary: Add to a pointer to step through memory, subtract two pointers to count elements, and see why an array turns into a pointer when you pass it to a function.
minutes: 25
difficulty: medium
tags: pointers, arrays
id: c-memory/pointer-arithmetic
mode: harness
---
You already know that `arr[i]` reaches the `i`-th element of an array. Underneath, C does pointer arithmetic: `arr[i]` means exactly `*(arr + i)`.

## Adding to a pointer

If `p` points to an `int`, then `p + 1` points to the **next int**, not the next byte. C multiplies by the size of the thing pointed to for you:

```c
int a[4] = {10, 20, 30, 40};
int *p = a;        // points at a[0]
*(p + 2)           // 30, the same as a[2]
p++;               // p now points at a[1]
```

On a machine where `int` is 4 bytes, `p + 1` is an address 4 higher. For a `char *` it would be 1 higher, for a `double *` 8 higher.

## Subtracting pointers

Two pointers into the same array can be subtracted. The answer is the number of **elements** between them, with type `ptrdiff_t` (from `<stddef.h>`):

```c
int *first = &a[0];
int *third = &a[2];
ptrdiff_t gap = third - first;   // 2
```

Only subtract pointers that point into the same array (or one past its end). Anything else is undefined behaviour.

## One past the end

A pointer may point one element past the last one, as long as you never read through it. This gives a neat way to write loops: `begin` points at the first element and `end` points just past the last.

```c
for (const int *p = begin; p != end; p++) {
    total += *p;
}
```

The range `[begin, end)` includes `begin` and excludes `end`. The number of elements is `end - begin`.

## Array decay

When you pass an array to a function, it **decays** into a pointer to its first element. These two declarations mean the same thing:

```c
int sum(int arr[], size_t n);
int sum(int *arr, size_t n);
```

The function receives only an address. It has no idea how long the array is, which is why a length always travels alongside. It also means `sizeof arr` inside the function gives the size of a pointer (8 bytes on a 64-bit machine), not of the array. Only in the function that declared the array does `sizeof a / sizeof a[0]` give the element count.

Because a pointer can point into the middle of an array, you can pass `a + 2` to hand a function "the array starting at element 2".

## Common mistakes

- Thinking `p + 1` adds one byte. It adds `sizeof *p` bytes.
- Dereferencing the `end` pointer.
- Using `sizeof` on a parameter to find the array length.

## Your turn

Write three functions using pointer arithmetic. Try to avoid writing `[ ]` in them.
