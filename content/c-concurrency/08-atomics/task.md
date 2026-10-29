Implement these in the starter (include `<stdatomic.h>` is already there):

- `long atomic_count(int threads, int iters)`: declare an `atomic_long` set to 0, start `threads` threads (at most 8) that each do `atomic_fetch_add(&c, 1)` `iters` times (write the thread function yourself), join them, and return the final value. No mutex.
- `void atomic_max(atomic_int *target, int v)`: raise `*target` to `v` if `v` is bigger, otherwise leave it. Use a compare-and-swap loop. It must be correct when many threads call it at the same time.
- `void spin_lock(atomic_flag *f)` and `void spin_unlock(atomic_flag *f)`: a spinlock built on `atomic_flag_test_and_set` and `atomic_flag_clear`.
