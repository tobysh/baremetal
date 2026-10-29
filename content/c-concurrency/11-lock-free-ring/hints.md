Both counters are `atomic_size_t`. Push reads `tail` (its own, relaxed) and `head` (acquire), checks `t - h == RING_SIZE`, writes `r->buf[t % RING_SIZE]`, then stores `t + 1` into `tail` with release.
===
Pop reads `head` (its own, relaxed) and `tail` (acquire). If `h == t` the ring is empty. Otherwise `*out = r->buf[h % RING_SIZE];` then `atomic_store_explicit(&r->head, h + 1, memory_order_release);`.
===
The order of the last two steps matters in both functions: do the slot access first, and publish the counter second. If you publish first, the other thread may read or overwrite the slot while you are still using it.
