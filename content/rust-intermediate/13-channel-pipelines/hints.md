`fan_out_sum`: create `let (tx, rx) = mpsc::channel();`, give every spawned thread `let tx = tx.clone();`, then `drop(tx);` and finish with `rx.iter().sum()`.
===
`squares_in_order`: send `(i, v as u64 * v as u64)` with `enumerate()`, collect `rx.iter()` into a `Vec`, `sort()` it (tuples sort by the first field), and strip the indices. `collect_from_producers`: make a `vec![Vec::new(); producers]` and push each `k` onto `result[p]` as messages arrive.
===
`pipeline`: make three channels. Stage 1 owns `rx1` and `tx2`: `for x in rx1 { tx2.send(x * 2).unwrap(); }`. Stage 2 owns `rx2` and `tx3`. Main sends the input into `tx1`, drops `tx1`, collects `rx3.iter()`, then joins both stage handles.
