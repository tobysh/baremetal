Separate *counting* from *writing*. Keep `n` as the number of characters the full map needs and increment it for every character, no matter how much room is left. Only the store `out[n] = c;` is guarded.
===
Guard each store with `if (cap && n < cap - 1)`. That one condition covers `cap == 0` (so `out` may be `NULL`), a short buffer, and keeps the last slot free for the NUL. Write it once as a tiny helper or repeat it for the `H` and the unit characters.
===
Per block: emit `'H'`, then loop `i` from 0 to `b->size / 16` emitting `b->is_free ? '.' : 'U'`. Use `first_block()` and `next_block(b)`; both already return `NULL` at the end (and `first_block()` returns `NULL` for an empty heap).
===
Terminate after the loop: `if (cap) out[n < cap ? n : cap - 1] = 0;`. When the map did not fit, `n` is larger than `cap - 1`, so the NUL goes into the last slot. Finally `return n;`.
