In `partition`, keep an index `i` for "the next slot for a small element". Scan `j` over `0 .. n-2`, and swap `a[j]` to `a[i]` whenever `a[j] <= pivot`.
===
Finish `partition` with `swap(&a[i], &a[n-1])` and `return i`. In `quicksort`, call `partition`, then sort the left part (length `p`) and the right part (start `a + p + 1`, length `n - p - 1`).
===
For `quickselect`, loop: partition the current range; if `p == k` return `a[p]`; if `k < p` shrink to the left part; else move the start to `a + p + 1`, decrease `n` by `p + 1` and decrease `k` by `p + 1`.
