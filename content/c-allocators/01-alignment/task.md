Implement these in the starter. `a` is an alignment and must be a power of two.

- `int is_pow2(size_t a)`: 1 if `a` is a non-zero power of two, else 0.
- `size_t align_up(size_t n, size_t a)`: the smallest multiple of `a` that is `>= n`. Return `SIZE_MAX` if `a` is not a power of two **or** the result does not fit in a `size_t`.
- `size_t align_down(size_t n, size_t a)`: the largest multiple of `a` that is `<= n`. Return `SIZE_MAX` if `a` is not a power of two.
- `size_t padding_for(uintptr_t addr, size_t a)`: bytes to skip from `addr` to reach the next multiple of `a` (0 if already aligned). `SIZE_MAX` for a bad `a`.
- `size_t layout(const size_t *size, const size_t *align, int n, size_t *offset)`: place `n` members in order like a C struct. Write each member's offset to `offset[i]` and return the total struct size, padded to the largest member alignment. Return 0 if `n <= 0`.

Hidden tests compare against a slow loop-based reference, against the compiler's own `offsetof` and `sizeof` for several structs, and probe values next to `SIZE_MAX`.
