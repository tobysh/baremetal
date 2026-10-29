A helper `static long find_slot(const OAMap *m, int key)` that returns the index of the slot holding `key` (or -1) makes `oa_get`, `oa_del` and the update path of `oa_put` easy.
===
In `find_slot`: start at `oa_hash(key) & (cap - 1)`. Repeat at most `cap` times: if the slot is `EMPTY`, stop (absent); if it is `USED` with the same key, return it; otherwise (a different key, or `DELETED`) move to `(i + 1) & (cap - 1)`.
===
`oa_put`: first call `find_slot`; if found, update the value and return. Otherwise probe again from the home slot for the first slot that is not `USED` (an `EMPTY` or `DELETED` one), write the pair there, set it `USED` and increment `count`. If you go all the way around without finding one, return -1.
