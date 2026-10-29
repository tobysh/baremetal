`count_up`: make `Arc::new(Mutex::new(0u64))`, clone the `Arc` into each thread with `move`, and `*c.lock().unwrap() += 1` in the loop. Join all handles, then read the value.
===
`Account` can hold `Arc<Mutex<i64>>`. `handle` is `Account { balance: Arc::clone(&self.balance) }`. In `withdraw`, take one guard: `let mut g = self.balance.lock().unwrap(); if *g >= n { *g -= n; true } else { false }`.
===
A poisoned `lock()` is `Err(e)`, and `e.into_inner()` returns the guard anyway: `*m.lock().unwrap_or_else(|e| e.into_inner())`.
