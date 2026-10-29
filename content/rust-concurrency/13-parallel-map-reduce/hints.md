Use `thread::scope(|s| { let handles: Vec<_> = items.chunks(size).map(|chunk| s.spawn(move || chunk.iter().fold(seed, |acc, x| reduce(acc, map(x))))).collect(); ... })`. Clone the seed on the calling side (`let seed = identity.clone();`) and move it in: only `U: Send` is promised, so a `&U` (which would need `U: Sync`) cannot be shared.
===
If the compiler complains the borrowed `map` may not live long enough, remember scoped threads are joined before `scope` returns, so shared borrows of function arguments are fine. Declare `let (map, reduce) = (&map, &reduce);` and use `move` closures to copy those references in.
===
Combine partials with `handles.into_iter().map(|h| h.join().unwrap()).fold(identity, |acc, p| reduce(acc, p))`. For `word_frequencies`: `map` builds a one-line `BTreeMap` with `entry(...).or_insert(0) += 1`, `reduce` moves the entries of one map into the other.
