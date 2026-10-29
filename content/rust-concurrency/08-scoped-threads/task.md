Complete these in the starter, using `thread::scope` (no `Arc`, no cloning of the data):

- `fn parallel_sum(data: &[u64], parts: usize) -> u64`: split `data` into at most `parts` chunks of equal size (the last may be shorter), sum each in its own scoped thread, and add the sums. Treat `parts == 0` as 1 and handle an empty slice.
- `fn increment_all(data: &mut [i32], parts: usize)`: add 1 to every element, with each scoped thread owning a disjoint `chunks_mut` piece. Same rules for `parts`.
- `fn min_max(data: &[i32]) -> Option<(i32, i32)>`: find the minimum in one scoped thread and the maximum in another; `None` if `data` is empty.
