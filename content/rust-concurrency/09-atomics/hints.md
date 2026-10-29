`count_atomic`: `let c = Arc::new(AtomicUsize::new(0));` clone it into each thread and call `c.fetch_add(1, Ordering::Relaxed)`. Read it with `c.load(Ordering::SeqCst)` after joining.
===
`next_id` is a one-liner: `NEXT_ID.fetch_add(1, Ordering::Relaxed)`. `record_max` is `cur.fetch_max(v, Ordering::Relaxed);`.
===
`Stop` holds an `AtomicBool`. `request` does `store(true, Ordering::Release)`, `is_requested` does `load(Ordering::Acquire)`. The worker: `let mut n = 0; loop { n += 1; if stop.is_requested() { break; } thread::yield_now(); } n`.
