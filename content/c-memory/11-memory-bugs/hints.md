Look at each function and ask: which blocks does it allocate, and where is each one freed? `make_label` allocates two blocks but returns only one. `sum_and_free` frees `values` before its last use. `release_person` frees one field twice and forgets another.
===
`make_label`: the scratch buffer is not returned, so free it before returning. `sum_and_free`: move the `free` to after the loop (compute the sum into a local variable first). `release_person`: list what `create_person` allocated (name, scores, the struct) and free each of those once.
===
```c
void release_person(Person *p) {
    free(p->name);
    free(p->scores);
    free(p);
}
```
Check that `make_label` frees `tmp` and `sum_and_free` returns a value it computed before the `free`.
