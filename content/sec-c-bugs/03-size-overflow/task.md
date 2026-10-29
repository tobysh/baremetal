Implement `int allocation_size(size_t count, size_t width, size_t *out)`: compute `count * width + 1`, returning 1 when representable and 0 otherwise. NULL `out` is failure. On failure do not change `*out`; do not allocate memory.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
