Complete these in the starter, using adaptors rather than index loops:

- `fn dot(a: &[i64], b: &[i64]) -> i64`: the sum of `a[i] * b[i]`. If the slices differ in length, only the overlapping part counts.
- `fn running_max(v: &[i32]) -> Vec<i32>`: element `i` is the largest of `v[0..=i]`. An empty slice gives an empty vector.
- `fn pairwise_diffs(v: &[i32]) -> Vec<i32>`: each element minus the one before it (`v[i+1] - v[i]`). Fewer than two items gives an empty vector.
- `fn chunk_sums(v: &[i32], n: usize) -> Vec<i32>`: the sum of each consecutive group of `n` items; the last group may be shorter. When `n` is 0, return an empty vector.
- `fn positions(v: &[i32], target: i32) -> Vec<usize>`: every index where `target` occurs.

The tests try many different slices, so write the general rule.
