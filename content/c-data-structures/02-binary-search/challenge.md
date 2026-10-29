**Challenge: binary search on an answer.**

Binary search does not need an array. It works on any yes/no question whose answer flips exactly once: `no, no, no, yes, yes, yes`. Write

```c
int first_true(int lo, int hi, int (*pred)(int, void *), void *ctx);
```

It returns the smallest `i` in `lo..hi-1` for which `pred(i, ctx)` is non-zero, or `hi` if there is none. The predicate is monotone: once it is true for some `i`, it is true for every larger value. `ctx` is passed to `pred` untouched.

- Call `pred` as few times as possible. The tests count the calls: for a range of `n = hi - lo` values you may call it at most `ceil(log2(n + 1))` times, so a linear scan fails even on a short range.
- Never call `pred` outside `lo..hi-1`.
- The range can be huge (`INT_MIN` to `INT_MAX`), so `(lo + hi) / 2` can overflow. Work out the middle in a way that cannot.
- `lo >= hi` means an empty range: return `lo` and do not call `pred`.
