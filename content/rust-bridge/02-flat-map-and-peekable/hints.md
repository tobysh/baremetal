For `run_lengths`: `let mut it = s.chars().peekable(); while let Some(c) = it.next() { let mut n = 1; while it.peek() == Some(&c) { it.next(); n += 1; } out.push((c, n)); }`.
===
`all_words`: `lines.iter().flat_map(|l| l.split_whitespace()).map(|w| w.to_lowercase()).collect()`. `split_by_sign`: `v.iter().partition(|&&x| x >= 0)`, with the tuple type written on the `let` or the return type, because `partition` needs to know which collection to build. Items are `&i32`, so use `.iter().copied()` first.
===
`first_big_square`: `v.iter().find_map(|&x| { let sq = x * x; if sq > limit { Some(sq) } else { None } })`. `numbers_in`: `items.iter().filter_map(|s| s.parse().ok()).collect()`; `parse::<i32>()` returns a `Result`, and `.ok()` turns it into an `Option`.
