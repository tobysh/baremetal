`count_common(a, na, b, nb)` returns how many values occur in both sorted arrays of distinct ints. Either length may be 0.

- Every comparison of values must go through `cmp3`. Do not compare array elements directly.
- Use at most `min(na + nb, small * (ceil(log2(big + 1)) + 1))` calls to `cmp3`, where `small` and `big` are the shorter and longer length. With an empty array that is 0 calls.
- Do not modify the input arrays.

The tests try balanced arrays, one tiny and one huge array in both orders, identical arrays, disjoint arrays and empty arrays, and compare the answer with a brute-force reference that does not use `cmp3`.
