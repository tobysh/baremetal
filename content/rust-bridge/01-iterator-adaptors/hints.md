`dot` is `a.iter().zip(b).map(|(x, y)| x * y).sum()`. `zip` already stops at the shorter slice, so you get the overlap rule for free.
===
`running_max`: `v.iter().scan(i32::MIN, |m, &x| { *m = (*m).max(x); Some(*m) }).collect()`. `pairwise_diffs`: `v.windows(2).map(|w| w[1] - w[0]).collect()`.
===
`chunk_sums` panics if you call `chunks(0)`, so return early when `n == 0`. Then `v.chunks(n).map(|c| c.iter().sum()).collect()`. For `positions`: `v.iter().enumerate().filter(|(_, &x)| x == target).map(|(i, _)| i).collect()`.
