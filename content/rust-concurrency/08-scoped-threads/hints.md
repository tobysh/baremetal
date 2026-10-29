Chunk size is `(data.len() + parts - 1) / parts`, at least 1 (use `.max(1)`). Then `thread::scope(|s| { let hs: Vec<_> = data.chunks(size).map(|c| s.spawn(move || c.iter().sum::<u64>())).collect(); hs.into_iter().map(|h| h.join().unwrap()).sum() })`.
===
`increment_all`: `thread::scope(|s| { for chunk in data.chunks_mut(size) { s.spawn(move || { for x in chunk.iter_mut() { *x += 1; } }); } });` The scope joins everything for you.
===
For `min_max`, return early with `None` if `data.is_empty()`. Otherwise `thread::scope(|s| { let lo = s.spawn(|| *data.iter().min().unwrap()); let hi = s.spawn(|| *data.iter().max().unwrap()); (lo.join().unwrap(), hi.join().unwrap()) })`.
