`ht_init` is a loop setting `t->buckets[i] = NULL`. In `ht_get`, compute `size_t b = hash_str(key) % BUCKETS;`, then walk `for (Entry *e = t->buckets[b]; e != NULL; e = e->next)` and compare with `strcmp(e->key, key) == 0`.
===
`ht_put` begins the same way as `ht_get`. If the walk finds the key, set `e->value = value; return 0;`. If it does not, `malloc` an `Entry`, then `malloc(strlen(key) + 1)` for the key and copy it. If the second allocation fails, free the entry before returning `-1`.
===
```c
e->next = t->buckets[b];   // new entry points at the old front
t->buckets[b] = e;         // and becomes the new front
```
For `ht_free`, save `Entry *next = e->next;` before calling `free(e->key); free(e);`, then move on with `e = next;`.
