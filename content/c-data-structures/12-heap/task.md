The `Heap` struct (a min-heap of ints in a fixed-capacity array) is given.

- `int heap_init(Heap *h, size_t cap)` allocates room for `cap` items. Returns `0`, or `-1` if `cap == 0` or allocation fails (leave `a` `NULL`).
- `int heap_push(Heap *h, int x)` inserts `x` and sifts up. Returns `-1` if the heap is full.
- `int heap_pop(Heap *h, int *out)` removes the smallest item into `*out` and restores the heap property. Returns `-1` if empty (leave `*out` alone).
- `int heap_peek(const Heap *h, int *out)` reads the smallest item without removing it. Returns `-1` if empty.
- `void heap_free(Heap *h)` frees the array and resets the heap.
- `int heap_sort(int *a, size_t n)` sorts `a` ascending by pushing all items into a temporary heap and popping them back. Returns `0`, or `-1` if allocation fails.

The tests inspect the array: after every operation `h->a[(i - 1) / 2] <= h->a[i]` must hold for every `i > 0`.
