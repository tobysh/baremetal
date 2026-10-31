Implement `Pack(a Allocator, items []string) (*Packed, error)` and the two methods.

- Allocate in this order: `data` (total bytes), `index` (`4*len(items)`), `scratch` (4 bytes). Even an empty `items` makes all three calls.
- Fill `data` with the items back to back, and `index[4*i:]` with the big-endian `uint32` end offset of item `i`, encoded through `scratch`.
- Free `scratch` before returning on every path where it was allocated.
- If any `Alloc` fails, free what was already allocated, return `nil` and the allocator's error unchanged.
- `(*Packed).Item(i) string` returns item `i`. `(*Packed).Release(a)` frees `data` and `index`.

The tests fail allocation number 1, 2 and 3 in turn, and check the leak count after each failure and after `Release`.
