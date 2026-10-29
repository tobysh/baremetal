`Mailbox` is in the starter: two plain ints `a` and `b` and an `atomic_int ready` (0 = empty, 1 = full). Implement:

- `void mailbox_send(Mailbox *m, int a, int b)`: write `a` and `b`, then publish with an atomic store of `1` to `ready` using **`memory_order_release`**.
- `int mailbox_try_receive(Mailbox *m, int *a, int *b)`: load `ready` with **`memory_order_acquire`**. If it is `0`, return `0` and leave `*a` and `*b` alone. If it is `1`, copy the two values out and return `1`.
- `void lock_acquire(atomic_flag *f)`: spin on `atomic_flag_test_and_set_explicit` with **`memory_order_acquire`**.
- `void lock_release(atomic_flag *f)`: `atomic_flag_clear_explicit` with **`memory_order_release`**.
