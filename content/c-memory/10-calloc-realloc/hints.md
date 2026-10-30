`new_zeroed` is one line: `calloc(n, sizeof(int))`.
===
In `vec_push`, check `v->len == v->cap`. Work out `new_cap`, call `realloc(v->data, new_cap * sizeof *v->data)` into a temporary pointer, return -1 if it is `NULL`, otherwise store it in `v->data` and update `v->cap`. Then `v->data[v->len++] = value;`. `vec_shrink` is the same dance with `v->len` as the size, plus the special case for an empty vector.
===
```c
if (v->len == v->cap) {
    size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
    int *tmp = realloc(v->data, new_cap * sizeof *tmp);
    if (tmp == NULL) {
        return -1;
    }
    v->data = tmp;
    v->cap = new_cap;
}
v->data[v->len++] = value;
return 0;
```
