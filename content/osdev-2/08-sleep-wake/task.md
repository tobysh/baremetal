The `Task` type (an `id` and a `state`, `READY` or `BLOCKED`) and the `WaitQueue` type (an array of 8 task pointers, a `head` index and a `count`) are in the starter. Implement the queue as a ring buffer:

- `void wait_init(WaitQueue *q)` makes `q` empty, whatever its fields held before.
- `int sleep_on(WaitQueue *q, Task *t)` appends `t` at the tail of `q`, sets `t->state` to `BLOCKED` and returns 0. It returns -1 and changes neither `t` nor `q` if `t` is not `READY` or `q` already holds 8 tasks.
- `Task *wake_one(WaitQueue *q)` removes the task that has waited longest, sets its state to `READY` and returns it. On an empty queue it returns `NULL` and changes nothing.

The tests use several queues at once, wrap the ring around its end, and refill queues after emptying them.
