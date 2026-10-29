Every function starts with `pthread_mutex_lock(&b->lock);` and ends with an unlock. `bb_put` waits with `while (b->count == BB_CAP) pthread_cond_wait(&b->not_full, &b->lock);`.
===
The ring: write at index `(b->head + b->count) % BB_CAP`, then `count++`. To remove, read `b->items[b->head]`, then `head = (head + 1) % BB_CAP` and `count--`. After changing the buffer, signal the *other* condition: `put` signals `not_empty`, `get` signals `not_full`.
===
`bb_get` waits with `while (b->count == 0 && !b->closed) pthread_cond_wait(&b->not_empty, &b->lock);`. After the loop, if `b->count == 0` the buffer must be closed: unlock and return 0. `bb_close` sets `closed = 1` and calls `pthread_cond_broadcast` on **both** conditions.
