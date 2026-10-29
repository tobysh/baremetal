Write a `swap` helper, then `heap_push` (put the item at index `len`, then sift up) and `heap_peek`.
===
`heap_pop`: save `a[0]` into `*out`, decrement `len`, move `a[len]` to `a[0]` (if any items remain), then sift down: repeatedly find the smallest of the node and its two children, and swap if a child is smaller.
===
`heap_sort`: if `n < 2` return 0. Otherwise `heap_init(&h, n)`, push each element, then pop into `a[0], a[1], ...`, and `heap_free`. The popped values come out smallest first, which is exactly ascending order.
