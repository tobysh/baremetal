`median`: `let mut s = v.to_vec(); s.sort(); let n = s.len();` then handle `0`, odd (`s[n / 2] as f64`) and even (`(s[n/2 - 1] + s[n/2]) as f64 / 2.0`). Add as `i64` first if you worry about overflow.
===
`top_n`: copy, `sort_by(|a, b| b.cmp(a))` (or `sort()` then `reverse()`), then `truncate(n)`. `insert_sorted`: `let i = match v.binary_search(&x) { Ok(i) | Err(i) => i }; v.insert(i, x);`. `unique_sorted`: copy, `sort()`, `dedup()`.
===
`ends`: `match v { [] => None, [x] => Some((*x, *x)), [first, .., last] => Some((*first, *last)) }`. `sort_people`: `people.sort_by(|a, b| b.1.cmp(&a.1).then_with(|| a.0.cmp(&b.0)))`. `then_with` only runs the second comparison when the first says equal.
