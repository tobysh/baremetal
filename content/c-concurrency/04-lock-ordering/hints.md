Handle the easy refusals first (same account, `amount <= 0`) before you lock anything. Locking `from` twice in one thread would freeze on its own.
===
Pick `first` and `second` by comparing `from->id` and `to->id`, lock `first` then `second`, and unlock in the reverse order.
===
Inside the locks, check `from->balance >= amount`. If it is true, subtract from `from` and add to `to`, setting `done = 1`. Unlock both, then `return done;`.
