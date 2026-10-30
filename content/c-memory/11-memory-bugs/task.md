Each of the three functions in the starter has one memory bug. Fix them without changing their results. `create_person` is correct; leave it as is.

- `char *make_label(const char *name, int id)` returns a new string like `"Ada#7"` that the caller frees. It must not leak anything else it allocates.
- `int sum_and_free(int *values, int n)` returns the sum of `values[0..n-1]` and frees the array (the caller gives up ownership). It must read the array only while it is still alive.
- `void release_person(Person *p)` frees everything `create_person` allocated for `p`: the name, the scores array and the `Person` itself. Each block exactly once.

The tests count allocations and frees, and detect a block being freed twice or used after it was freed.
