---
title: Allocation size overflow
summary: Check a multiplication and an added terminator before an allocation size can wrap.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-c-bugs/size-overflow
mode: harness
---
A common allocation is `malloc(count * width + 1)`. Unsigned arithmetic wraps modulo one more than `SIZE_MAX`: the expression can become tiny while later code still believes it allocated `count` elements. Checking the result after multiplying is too late.

Work backward from the maximum permitted result. Reserving one trailing byte leaves `SIZE_MAX - 1` bytes for elements. When `width` is nonzero, the product fits exactly when `count <= (SIZE_MAX - 1) / width`. Division happens before multiplication and cannot overflow. A zero width is valid here and yields a one-byte allocation size.

This exercise computes a size without allocating, so even enormous values are safe to test. The function also leaves the caller's output untouched on failure. In a real allocation path, check `malloc` separately: representable does not mean affordable. Apply quotas as well as arithmetic checks. Signed overflow follows different rules and is undefined behaviour, which is another reason to use `size_t` for sizes and validate conversions at the boundary.

## The unsafe pattern

```c
size_t bytes = count * width + 1;
void *p = malloc(bytes);
```

The multiplication can wrap before malloc sees the request. A small allocation does not justify a later count-element write.
