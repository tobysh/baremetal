Complete the handle API around `Counter` (given; always create counters with `Counter::new`):

- `counter_new(step: u32) -> *mut Counter`: a heap counter with total 0, ownership moved to the caller.
- `unsafe counter_bump(c: *mut Counter) -> u64`: add `step` to the total (wrapping) and return the new total. A null handle returns 0.
- `unsafe counter_total(c: *const Counter) -> u64`: the current total; null gives 0.
- `unsafe counter_free(c: *mut Counter)`: drop the counter; null is a no-op.
- `unsafe counter_into_inner(c: *mut Counter) -> u64`: consume the handle, returning its final total and freeing it. Null returns 0.

The tests count live counters, so every handle must be freed exactly once.
