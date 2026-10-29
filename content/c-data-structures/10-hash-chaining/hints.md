Write `sm_get` first: compute the bucket, walk the chain with `strcmp`. A helper `static Entry *lookup(const StrMap *m, const char *key)` is reused by `sm_put`.
===
`sm_put` for a new key: `Entry *e = malloc(sizeof *e); e->key = malloc(strlen(key) + 1); strcpy(e->key, key);` then link it at the head of `m->buckets[b]`. Increment `count`, then call a `static int grow(StrMap *m)` if `count > 2 * nbuckets`.
===
`grow`: allocate a `calloc`'d array of `2 * nbuckets` heads; for each old bucket, pop entries one by one and push each onto the head of its new bucket `str_hash(e->key) % new_n`; free the old array and update the fields. For `sm_del`, use `Entry **link = &m->buckets[b]` and advance it with `link = &(*link)->next`.
