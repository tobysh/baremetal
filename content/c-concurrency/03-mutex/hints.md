Every function that touches `value` does `pthread_mutex_lock(&c->lock);`, then the work, then `pthread_mutex_unlock(&c->lock);`.
===
`counter_add_if_below` needs one lock around both the comparison and the addition. Remember to unlock on both the success and the failure path.
===
`count_in_parallel`: `Counter c; counter_init(&c); Job job = {&c, iters};` then create `threads` threads with `&job` as the argument (they can all share one read-only `Job`). Join them all, read `counter_get(&c)` into a local, destroy, return the local.
