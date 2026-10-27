`vec_init` just sets three fields: `v->data = NULL; v->len = 0; v->cap = 0;`. `vec_free` calls `free(v->data)` and then resets the same three fields.
===
In `vec_push`, if `v->len == v->cap`, compute `new_cap = v->cap == 0 ? 4 : v->cap * 2`, call `realloc` into a temporary pointer, and return `-1` if it is `NULL`. Only then store it in `v->data` and update `v->cap`.
===
```c
int vec_pop(IntVec *v, int *out) {
    if (v->len == 0) {
        return -1;
    }
    v->len--;
    *out = v->data[v->len];
    return 0;
}
```
Popping does not need to shrink the block. Just lowering `len` is enough.
