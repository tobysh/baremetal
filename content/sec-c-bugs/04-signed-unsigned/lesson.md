---
title: Signed lengths at a boundary
summary: Reject negative lengths before converting them to size_t or copying bytes.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-c-bugs/signed-unsigned
mode: harness
---
A parser may return `-1` for an error. Passing that value to `memcpy`, whose length is `size_t`, converts it into a huge unsigned number. A comparison such as `len <= cap` can also perform a conversion before you expected it. Follow the value through its type changes.

Keep the signed input signed until you have established `len >= 0`. Only then convert it to `size_t` and compare it with the destination capacity. Our interface returns the copied length, or -1 for failure. Success with length zero is valid even for NULL pointers.

The source is guaranteed to have `len` readable bytes when `len` is positive; this function can validate the destination capacity but cannot infer the source allocation size from a pointer. Production interfaces should carry both bounds when that guarantee is unavailable. This separation between a caller guarantee and a checked condition helps make a review precise. Do not fix a negative length by taking its absolute value: `INT_MIN` cannot be negated in an `int`, and an error is not a valid request.

## The unsafe pattern

```c
int len = parse_length(); /* may return -1 */
memcpy(dst, src, (size_t)len);
```

The cast turns an error into a huge request. Reject negative values while they are still signed.
