---
title: Binary Search
summary: Put everything together, with registers, comparisons, scaled addressing and careful bounds, to search a sorted array in logarithmic time.
minutes: 35
difficulty: hard
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

## Common mistakes

- Using `hi = n - 1` with an `lo < hi` loop, which never examines the last element. Choose one convention (half-open here) and stick to it.
- Updating `lo = mid` instead of `mid + 1`, which can loop forever.
- Mixing signed and unsigned jumps: one for the data comparison, another for the index comparison.
- Overwriting `rdi` or `rdx` inside the loop, losing the array or the key.
- Reading `a[mid]` when `n = 0`.

Now implement `binary_search`.
