The starter is a working first-fit heap. `heap_alloc`, `heap_free`, `heap_stats`, `heap_init` and `heap_set_policy` are supplied and must keep their behaviour. Your part is `find_block` and `run_workload`.

1. `static Block *find_block(size_t need)` receives the already-rounded payload size and returns the chosen free block, or `NULL` if none fits. It must honour `policy`:
   - `FIRST_FIT`: address order, return the first free block with `size >= need`.
   - `BEST_FIT`: look at **every** block and return the free block with the smallest `size >= need`. A strictly smaller block replaces the candidate, so the lowest address wins ties.
   - In both cases add 1 to `steps` for every block examined, used or free, including the chosen one. First fit stops counting at the chosen block; best fit, and any search that finds nothing, examines every block.
   - `find_block` must not modify any block. Splitting stays in `heap_alloc`.
2. `heap_alloc(n)` (given) returns `NULL` without searching for `n == 0` (not a failure) and for `n` larger than the heap (a failure, zero steps). Otherwise it calls `find_block`; a `NULL` result counts as a failure. `heap_init` resets both counters and selects first fit.
3. `WorkResult run_workload(Policy p, const Op *ops, int n)` uses a fresh 2048-byte, 16-aligned heap with policy `p` on every call (no state may leak between calls). Each `Op` is `{0, id, size}` (allocate `size` bytes, remember the pointer under `id`) or `{1, id, 0}` (free the pointer remembered under `id`). Ids are `0..31`. Skip a free whose id holds no pointer (its allocation failed), and clear the id after a successful free. Return `failed` (failed allocations), `steps`, and `largest_free` and `free_blocks` as they are after the last operation.

Finally, in the comment above `run_workload`, write two sentences about which policy did better on the public example and why. The tests cannot grade this.

Hidden tests compare pointers, counters and workload results against an independent model, for both policies, over seeded random workloads of several heap sizes and the trace from the lesson. A step count that is off by one fails.
