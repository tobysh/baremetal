Replace the direct call `handle_connection(stream)` with `thread::spawn(move || handle_connection(stream))`. The loop should then immediately go back to `accept`.
===
Collect the `JoinHandle` returned by each `spawn` into a `Vec`. After the accept loop finishes, loop over the vector and call `.join()` on every handle.
===
Shape: `let mut handles = Vec::new(); for _ in 0..max_connections { if let Ok((stream, _)) = listener.accept() { handles.push(thread::spawn(move || handle_connection(stream))); } } for h in handles { let _ = h.join(); }`
