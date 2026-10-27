`IntVec`, `vec_init`, `vec_push` and `vec_free` are provided in the starter. Implement these three functions:

- `void vec_map(IntVec *v, int (*f)(int))` replaces each element `x` with `f(x)`, in place.
- `size_t vec_filter(IntVec *v, int (*keep)(int))` removes, in place, every element for which `keep(x)` returns `0`. The remaining elements stay in their original order. Return how many were removed. `cap` is left unchanged.
- `int vec_reduce(const IntVec *v, int init, int (*f)(int acc, int x))` returns the result of starting with `acc = init` and then doing `acc = f(acc, x)` for each element from first to last. An empty vector returns `init`.

None of these may allocate memory or change `data` (the pointer itself).
