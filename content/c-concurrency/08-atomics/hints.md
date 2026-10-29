For `atomic_count`, make a small struct (or use a global `atomic_long` plus `iters`), and give the thread function a loop of `atomic_fetch_add(&c, 1)`. Pass a pointer to the shared `atomic_long` through the thread's argument.
===
`spin_lock`: `while (atomic_flag_test_and_set(f)) { }`. The call returns the previous value, so a true result means "somebody already had it". `spin_unlock`: `atomic_flag_clear(f);`.
===
```c
void atomic_max(atomic_int *target, int v) {
    int old = atomic_load(target);
    while (old < v && !atomic_compare_exchange_weak(target, &old, v)) {
    }
}
```
When the exchange fails, `old` is refreshed with the current value, and the loop checks `old < v` again.
