Implement:

- `void add128(const uint64_t *a, const uint64_t *b, uint64_t *out)` adds two 128-bit numbers (two limbs each, low limb first) and stores the 128-bit result in `out`, wrapping on overflow. `out` may be the same pointer as `a` or `b`.
- `uint64_t mulhi(uint64_t a, uint64_t b)` returns the upper 64 bits of the 128-bit product.
- `unsigned add_n(const uint64_t *a, const uint64_t *b, uint64_t *out, size_t n)` adds two `n`-limb numbers and returns the final carry (0 or 1). `n` can be 0 (carry 0).
