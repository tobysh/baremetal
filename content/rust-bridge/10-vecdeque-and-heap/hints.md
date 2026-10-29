`josephus`: `let mut q: VecDeque<usize> = (1..=n).collect(); while !q.is_empty() { q.rotate_left((k - 1) % q.len()); out.push(q.pop_front().unwrap()); }`. Rotating by `k - 1` brings the k-th person to the front.
===
`k_smallest`: `let mut h = BinaryHeap::new(); for &x in v { h.push(x); if h.len() > k { h.pop(); } }` then `h.into_sorted_vec()` (ascending). With `k == 0` everything gets popped, giving an empty result. `run_tasks`: push `(p, name.to_string())` and pop until empty.
===
`window_maxima`: keep `dq: VecDeque<usize>` of indices. For each `i`: pop from the back while `v[*dq.back().unwrap()] <= v[i]`, push `i`; pop from the front if `*dq.front().unwrap() + w <= i`; once `i + 1 >= w`, push `v[dq[0]]` to the output.
