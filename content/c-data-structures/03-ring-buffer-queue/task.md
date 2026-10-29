Implement a fixed-capacity FIFO queue of ints (the struct is given):

- `int q_init(Queue *q, int cap)` allocates a buffer of `cap` ints. Returns `0` on success, `-1` if `cap <= 0` or `malloc` fails (leave `q->buf` as `NULL` then).
- `int q_push(Queue *q, int x)` adds `x` at the back. Returns `-1` if the queue is full (nothing changes).
- `int q_pop(Queue *q, int *out)` removes the oldest item into `*out`. Returns `-1` if the queue is empty (leave `*out` alone).
- `int q_peek(const Queue *q, int *out)` reads the oldest item without removing it. Returns `-1` if empty.
- `int q_size(const Queue *q)` returns the number of stored items.
- `void q_free(Queue *q)` frees the buffer and sets `buf` to `NULL` and `len` to 0.

Never move items around: use `head` and `len` with modular arithmetic. The tests push and pop through many wrap-arounds.
