---
title: Binary Search
summary: Put everything together, with registers, comparisons, scaled addressing and careful bounds, to search a sorted array in logarithmic time.
minutes: 35
difficulty: hard
tags: algorithms, branching, memory
id: asm-first-steps/binary-search
mode: harness
---
This capstone uses every tool from the track: arguments in registers, loads with scaled addressing, signed comparisons, branches, loops and several return paths.

## The algorithm

A sorted array lets you discard half the candidates with every comparison. Keep a half-open range `[lo, hi)` of positions that might still contain the key:

```c
long find(const long *a, size_t n, long key) {
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (a[mid] == key) return mid;
        if (a[mid] < key)  lo = mid + 1;   // key is in the upper half
        else               hi = mid;       // key is in the lower half
    }
    return -1;
}
```

A half-open range means `hi` is *one past* the last candidate, so an empty array is simply `lo == hi == 0` and the loop never runs. Because the range shrinks every time, the loop always terminates.

### Why it is correct: the invariant

Whenever you write a loop, find a sentence that is true every time you reach the top. For binary search it is:

> If `key` is in the array, its index is in `[lo, hi)`.

It is true at the start, because `[0, n)` covers the whole array. Each step keeps it true. If `a[mid] < key`, then since the array is sorted, everything at or below `mid` is too small, so the key can only be in `[mid + 1, hi)`. If `a[mid] > key`, everything at or above `mid` is too big, so the key can only be in `[lo, mid)`. The range gets strictly smaller each time, so the loop ends, and when it ends with `lo == hi` the range is empty: the key is not there.

This is also why the two off-by-one choices (`mid + 1` and `mid`) are not interchangeable. If you used `lo = mid` the range might not shrink, and the loop could repeat forever.

### Tracing two searches

Use the sorted array `a = {2, 5, 8, 12, 16, 23, 38}`, so `n = 7`.

Searching for `23` (present):

| lo | hi | mid | `a[mid]` | decision |
|---|---|---|---|---|
| 0 | 7 | 3 | 12 | 12 < 23, so `lo = 4` |
| 4 | 7 | 5 | 23 | equal: return 5 |

Searching for `9` (absent):

| lo | hi | mid | `a[mid]` | decision |
|---|---|---|---|---|
| 0 | 7 | 3 | 12 | 12 > 9, so `hi = 3` |
| 0 | 3 | 1 | 5 | 5 < 9, so `lo = 2` |
| 2 | 3 | 2 | 8 | 8 < 9, so `lo = 3` |
| 3 | 3 | | | `lo >= hi`: range empty, return -1 |

Each step halves the range, so an array of one billion elements needs at most about 30 steps, where a straight scan would need up to a billion. That is the difference between O(log n) and O(n).

## Computing the midpoint

`(lo + hi) / 2` can overflow for huge values, so write it as `lo + (hi - lo) / 2`. Dividing by two is a right shift:

```asm
mov rax, r8        # hi
sub rax, rcx       # hi - lo
shr rax, 1         # (hi - lo) / 2   (unsigned)
add rax, rcx       # + lo
```

## Choosing registers

You have plenty of caller-saved registers, so no stack is needed. A possible plan:

| Register | Holds |
|---|---|
| `rdi` | the array pointer `a` (unchanged) |
| `rdx` | the `key` |
| `rcx` | `lo` |
| `r8` | `hi` |
| `rax` | `mid`, then the return value |
| `r9` | `a[mid]` |

The arguments arrive as `a` in `rdi`, `n` in `rsi` and `key` in `rdx`.

## Loading and comparing

```asm
mov r9, [rdi + rax*8]     # r9 = a[mid]
cmp r9, rdx               # compare a[mid] with key
je  .Lfound
jl  .Lgo_right            # a[mid] < key  (signed: the data are long)
```

The array holds signed `long`s, so use `jl`; but positions (`lo`, `hi`, `mid`) are `size_t`, so compare them with `jae`.

## Returning -1

`mov rax, -1` returns the signed value `-1`. Since the function returns `long`, the caller sees `-1` in 64 bits.

## How to approach a bigger function

This is the first function large enough that writing straight into assembly is a mistake. A method that works well:

1. **Write the C first** (the lesson gave it to you) and get the logic right in your head.
2. **Assign registers** in a table before writing a single instruction, as above. Note which arguments you must keep unchanged.
3. **Give every C statement its own block of assembly with a comment** showing the C it came from. When something is wrong you can compare the two side by side.
4. **Name your labels after intent**: `.Lloop`, `.Lfound`, `.Lgo_right`, `.Lnot_found`. Labels like `.L1` and `.L2` are hard to debug.
5. **Check each exit.** There are three: found (return `mid`), not found (return -1) and the empty-array case, which falls out of the loop test. Each must set `rax` before `ret`.

Structure that keeps the loop tidy is a single comparison at the top (`lo >= hi` exits), then one three-way split on `a[mid]`. The three branches either return or update a bound and jump back to the top. If you find yourself writing a `jmp` that goes somewhere unexpected, stop and redraw the flow on paper.

## Common mistakes

- Using `hi = n - 1` with an `lo < hi` loop, which never examines the last element. Choose one convention (half-open here) and stick to it.
- Updating `lo = mid` instead of `mid + 1`, which can loop forever.
- Mixing signed and unsigned jumps: one for the data comparison, another for the index comparison.
- Overwriting `rdi` or `rdx` inside the loop, losing the array or the key.
- Reading `a[mid]` when `n = 0`.

Now implement `binary_search`.
