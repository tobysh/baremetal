The data fields are ordinary variables. The only atomic is `ready`. Write the fields first, then `atomic_store_explicit(&m->ready, 1, memory_order_release);` as the very last step.
===
For the receiver: `if (atomic_load_explicit(&m->ready, memory_order_acquire) == 0) return 0;`. Only after that line may you read `m->a` and `m->b`.
===
```c
void lock_acquire(atomic_flag *f) {
    while (atomic_flag_test_and_set_explicit(f, memory_order_acquire)) { }
}
void lock_release(atomic_flag *f) {
    atomic_flag_clear_explicit(f, memory_order_release);
}
```
