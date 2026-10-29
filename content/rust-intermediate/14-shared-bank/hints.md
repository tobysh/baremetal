`Bank::new` is `Bank { accounts: balances.iter().map(|&b| Mutex::new(b)).collect() }`. `balance` uses `self.accounts.get(i).map(|m| *m.lock().unwrap())`. `total` locks each account one at a time.
===
In `transfer`, do the three argument checks first. Then `let (lo, hi) = if from < to { (from, to) } else { (to, from) };` and lock `lo` then `hi`. Pick source and destination out of the two guards with `if from < to { (&mut *first, &mut *second) } else { (&mut *second, &mut *first) }`.
===
Check the funds *after* both locks are held, otherwise another thread could spend the money between your check and your update. `parallel_deposits`: `let b = Arc::clone(bank);` inside the loop, `thread::spawn(move || b.deposit(account, amt))`, collect the handles, join them.
