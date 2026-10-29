`BoundedBuffer` and `bb_init` are in the starter (capacity `BB_CAP` is 4, items are `int`, `items` is a ring: the oldest item is at `head`). Implement:

- `void bb_put(BoundedBuffer *b, int item)`: wait while the buffer is full, then append the item and wake a waiting consumer. (You will not be called after `bb_close`.)
- `int bb_get(BoundedBuffer *b, int *out)`: wait while the buffer is empty **and** not closed. If an item is available, remove the oldest one into `*out`, wake a waiting producer and return `1`. If the buffer is empty and closed, return `0`.
- `void bb_close(BoundedBuffer *b)`: mark the buffer closed and wake **all** waiting threads.

Items must come out in the order they went in.
