`names.into_iter().map(|n| thread::spawn(move || format!("hello, {n}"))).collect::<Vec<_>>()` gives the handles; join them afterwards.
===
For `indexed_squares`, `(0..n).map(|i| thread::spawn(move || i * i))` works because `i` is `Copy`. Collect the handles before joining.
===
`spawn_range_sum` returns the handle itself: `thread::spawn(move || (start..start + len).sum())`. The closure moves `start` and `len` in.
