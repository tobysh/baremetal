---
title: Binary Search
summary: Find values in sorted arrays with lower and upper bounds.
minutes: 25
difficulty: easy
tags: searching, arrays, algorithms
id: c-data-structures/binary-search
mode: harness
---
Looking for a name in a phone book, you do not start at page one. You open the middle, see whether your name comes before or after, and throw away half the book. Repeat and you find it in about 10 steps even among a thousand pages. That is **binary search**: on a **sorted** array of `n` items it needs only about log2(n) comparisons, where a linear scan needs up to `n`.

## The invariant

Keep a half-open range `[lo, hi)` that is guaranteed to contain the answer position:

```c
int lo = 0, hi = n;
while (lo < hi) {
    int mid = lo + (hi - lo) / 2;   // not (lo + hi) / 2: that can overflow
    if (a[mid] < x) lo = mid + 1;   // everything up to mid is too small
    else            hi = mid;       // mid might be the answer
}
return lo;
```

When the loop ends `lo == hi`, and that is the **first index whose value is >= x** (or `n` if every value is smaller). This is called `lower_bound`.

## Duplicates

With repeated values, "find x" is ambiguous: which copy? The two bounds answer it cleanly:

- `lower_bound(x)`: first index with `a[i] >= x`.
- `upper_bound(x)`: first index with `a[i] > x`.

All copies of `x` sit in `[lower_bound, upper_bound)`, so their count is the difference. If `x` is absent, both bounds are equal: the position where `x` *would* be inserted.

## Common mistakes

- Using a `<=` comparison in the wrong branch, which makes `lower_bound` and `upper_bound` swap roles. Change only that one comparison to get the other function.
- `hi = n - 1` with `while (lo <= hi)` is another valid style, but it needs extra care with empty arrays and with `mid - 1` going negative.
- Running binary search on an unsorted array: it silently returns nonsense.

Write `lower_bound`, `upper_bound` and `count_equal`.
