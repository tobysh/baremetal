`Range` is defined in the starter. Implement:

- `void *sum_worker(void *arg)`: `arg` points to a `Range`. Add up `a[lo]` through `a[hi - 1]` into a `long` and return it as `(void *)(intptr_t)total`. An empty range (`lo >= hi`) returns `0` the same way.
- `long parallel_sum(const int *a, int n, int nthreads)`: sum all `n` items using `nthreads` threads (1 to 8) and return the total. Split with `chunk = (n + nthreads - 1) / nthreads`, clamp each range to `n`, join every thread and add up what they return. `n` may be 0.
