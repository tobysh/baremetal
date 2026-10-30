`distinct_baseline(const int *a, int n)` returns how many different values `a[0..n-1]` contains. Values are always in `-100..100`. `n` may be `0`.

Fix `distinct_fast` so that for every such input it returns exactly what `distinct_baseline` returns.

- Do not change `distinct_baseline` or the work counter `bm_work`.
- `distinct_fast` must add `1` to `bm_work` for each element it reads and `1` for each table slot it clears or resets. The total must stay at or below `2 * n + 256`.
- `a` must not be modified.

Many arrays are tried, including empty, single-element, all-equal, all-different, negative values and the extremes `-100` and `100`. Compare the two functions in `main` on the same inputs after each change.
