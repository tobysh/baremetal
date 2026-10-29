`run_guarded`: `match thread::spawn(f).join() { Ok(v) => Ok(v), Err(payload) => Err(message_from(payload)) }`. Write a small helper that tries `downcast_ref::<&str>()`, then `downcast_ref::<String>()`.
===
`collect_ordered`: first collect all the handles: `(0..n).map(|i| { let f = Arc::clone(&f); thread::spawn(move || f(i)) }).collect::<Vec<_>>()`. Only then loop over the handles in order and `join()` each one, so every thread is joined even after you have seen an error.
===
Keep `let mut first_err: Option<String> = None;` and fill it only if it is still `None`, so the lowest index wins. Match `h.join()` as `Ok(Ok(v))`, `Ok(Err(e))` and `Err(_)`. `parse_chunks` is the same shape: each thread returns `Result<Vec<i64>, String>` built with `map_err(|e| format!("chunk {i}: {e}"))`.
