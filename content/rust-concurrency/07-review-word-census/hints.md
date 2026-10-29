Make `let workers = workers.max(1);` and build one `Vec<String>` per worker: `buckets[i % workers].push(line)`.
===
Per worker: `let tx = tx.clone(); let total = Arc::clone(&total); thread::spawn(move || { ...count into a HashMap...; *total.lock().unwrap() += words; tx.send(map).unwrap(); })`. Remember `drop(tx)` after the loop.
===
Merge with `for map in rx { for (w, c) in map { *merged.entry(w).or_insert(0) += c; } }`, then return `(merged, *total.lock().unwrap())`.
