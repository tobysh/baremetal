`lockstep`: share `Arc<Barrier>` and `Arc<Mutex<usize>>`. Each thread keeps a local `Vec` and for every round does `*counter.lock().unwrap() += 1; barrier.wait(); seen.push(*counter.lock().unwrap());`. Return the joined Vecs in thread order.
===
There is a catch in `lockstep`: a fast thread must not start round `r + 1` and add to the counter before a slow thread has read its round `r` value. Add a second `barrier.wait()` after reading, so reads and writes are in separate phases.
===
`leader_count`: `if barrier.wait().is_leader() { 1 } else { 0 }` per round, summed over all threads. For `ensure_init`: `INIT.call_once(|| { INIT_RUNS.fetch_add(1, Ordering::SeqCst); });`. For `config`: `CONFIG.get_or_init(|| { CONFIG_BUILDS.fetch_add(1, Ordering::SeqCst); String::from("config-v1") })`.
