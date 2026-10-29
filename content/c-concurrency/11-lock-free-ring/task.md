`Ring` and `ring_init` are in the starter (`RING_SIZE` is 8). Implement:

- `int ring_push(Ring *r, int value)`: called only by the producer thread. If the ring is full return `0` and change nothing. Otherwise store `value` in the next slot, publish it, and return `1`.
- `int ring_pop(Ring *r, int *out)`: called only by the consumer thread. If the ring is empty return `0` and leave `*out` alone. Otherwise remove the oldest value into `*out` and return `1`.

Use no mutex. Use `memory_order_acquire` for loading the other thread's counter and `memory_order_release` for publishing your own, as in the lesson. The ring holds exactly `RING_SIZE` items, and values come out in the order they went in.
