`level_of`: `let word = line.split_whitespace().next().unwrap_or("");` then `match word { "INFO" | "WARN" | "ERROR" => Ok(word), _ => Err(LogError::BadLine(number)) }`.
===
In `summarize`, for each `(ci, chunk)` in `lines.chunks(size).enumerate()`, clone the `Arc<Mutex<BTreeMap>>`, clone the `Sender`, copy the chunk with `to_vec()`, and compute the first line number as `ci * size + 1`. Each worker loops over its lines with `enumerate()`.
===
In a worker, `Ok(level) => { *counts.lock().unwrap().entry(level.to_string()).or_insert(0) += 1; }` and `Err(LogError::BadLine(n)) => tx.send(n).unwrap()`. After `drop(tx)` and joining every handle, take `rx.iter().min()`; if it is `Some(n)`, return `Err(LogError::BadLine(n))`, otherwise clone the map out of the mutex.
