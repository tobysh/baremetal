Build the mask once with `1u << n`, then each function is one short expression using that mask.
===
Set uses `|`, clear uses `&` with the flipped mask `~mask`, toggle uses `^`. For test, AND with the mask and compare the result to zero so you return exactly 1 or 0.
===
```c
uint32_t set_bit(uint32_t x, int n) {
    return x | (1u << n);
}
```
`clear_bit` is `x & ~(1u << n)`, `toggle_bit` is `x ^ (1u << n)`, and `test_bit` is `(x & (1u << n)) != 0`.
