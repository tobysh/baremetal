Each function is one loop over `v->data[0..v->len)`. `vec_map` stores `f(v->data[i])` back into `v->data[i]`. `vec_reduce` keeps a local `acc` and updates it with `f(acc, v->data[i])`.
===
For `vec_filter`, use two indices. `read` walks every element; `write` starts at 0. When `keep(v->data[read])` is non-zero, copy that element to `v->data[write]` and increment `write`. Elements you skip are simply overwritten later. Afterwards the number removed is `old_len - write`, and the new `len` is `write`.
===
```c
size_t write = 0;
for (size_t read = 0; read < v->len; read++) {
    if (keep(v->data[read])) {
        v->data[write++] = v->data[read];
    }
}
size_t removed = v->len - write;
v->len = write;
return removed;
```
