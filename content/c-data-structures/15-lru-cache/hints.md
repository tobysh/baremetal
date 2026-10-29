Write the small helpers first: `list_unlink(c, n)`, `list_push_front(c, n)`, `find(c, key)` (walk the hash chain), `chain_remove(c, n)`. Test each mentally with a one-node list.
===
`lru_get`: `n = find(c, key)`; if NULL return -1; otherwise `list_unlink(c, n); list_push_front(c, n);` and return the value. `lru_put` on an existing key does the same after updating `val`.
===
For a new key in `lru_put`: allocate a node, insert it at the head of its hash chain (`n->hnext = c->buckets[b]; c->buckets[b] = n;`), push it on the list front, `count++`. Then `if (count > cap)`: `LNode *victim = c->tail; list_unlink(c, victim); chain_remove(c, victim); free(victim); count--;`. In `list_unlink`, fix `head` and `tail` when the node is at either end.
