In the starter, `heap_alloc` calls `find_block(need)`. Complete it, and write `run_workload`.

1. `find_block(need)` must honour `policy`:
   - `FIRST_FIT`: address order, stop at the first free block with `size >= need`.
   - `BEST_FIT`: look at **every** block, remember the free one with the smallest `size >= need` (strictly smaller wins, so the lowest address wins ties).
   - In both cases add 1 to `search_steps` for every block examined. First fit stops counting at the chosen block; with no fit it examines all blocks.
2. `heap_alloc` (given) updates `failed_allocs`; `heap_stats` (given) reports the counters. `heap_init` resets them and selects first fit.
3. `WorkResult run_workload(Policy p, const Op *ops, int n)`: use a fresh 2048-byte, 16-aligned heap with policy `p`. Each `Op` is either `{0, id, size}` (allocate `size` bytes and remember the pointer under `id`) or `{1, id, 0}` (free the pointer remembered under `id`). Ids are `0..31`. Skip a free whose id holds no pointer (its allocation failed). Return `failed` (failed allocations), `steps`, and `largest_free` and `free_blocks` at the end.

Finally, in a comment above `run_workload`, write two sentences explaining which policy did better on the public example and why. This is for you; the tests cannot grade it.

Hidden tests compare pointers, counters and workload results to a segment model, for both policies, over seeded random workloads and the trace described in the lesson.
