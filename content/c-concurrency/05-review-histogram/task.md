`Task` is defined in the starter. Implement:

- `void *hist_worker(void *arg)`: `arg` is a `Task`. Count the bytes `data[lo]` to `data[hi - 1]` into a **local** array of 256 `long`s, then lock `*lock`, add each local count to `shared`, unlock, and return `NULL`.
- `void parallel_histogram(const unsigned char *data, size_t n, int nthreads, long hist[256])`: set all 256 counters in `hist` to 0, then split `data` into `nthreads` (1 to 8) slices with `chunk = (n + nthreads - 1) / nthreads`, clamping to `n`, run `hist_worker` on each, and join them all. `n` may be 0.
