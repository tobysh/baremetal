`Counter` and the `Job` struct are defined in the starter. Implement:

- `void counter_init(Counter *c)`: set `value` to 0 and initialise the mutex.
- `void counter_destroy(Counter *c)`: destroy the mutex.
- `void counter_add(Counter *c, long delta)`: add `delta` to the value, safely from any thread.
- `long counter_get(Counter *c)`: read the value, safely.
- `int counter_add_if_below(Counter *c, long delta, long limit)`: if `value + delta <= limit`, add `delta` and return `1`. Otherwise change nothing and return `0`. The check and the add must be one atomic step.
- `void *incr_worker(void *arg)`: `arg` is a `Job`. Call `counter_add(job->counter, 1)` `job->iters` times and return `NULL`.
- `long count_in_parallel(int threads, int iters)`: make a counter, start `threads` threads (at most 8) running `incr_worker`, join them, and return the final value. Destroy the counter before you return.
