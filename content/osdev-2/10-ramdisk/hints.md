A request is valid when every byte it touches is inside `[0, 512)`. Decide that **once**, before the copy loop, and return -1 straight away if it fails. If the loop discovers a problem half-way, part of the copy has already happened.
===
The tempting check `offset + len > 512` breaks for huge values: `size_t` addition wraps, so `10 + (size_t)-5` is 5. The test cases include exactly that. Rearrange the inequality so that nothing is ever added.
===
Check `offset` on its own first: anything above 512 is invalid. After that, `512 - offset` is a safe number (0 to 512), and `len` must not exceed it. Mind the boundary: `offset == 512` is allowed with `len == 0`.
===
The same shape for a different bounded buffer, a 4096-byte message area:

```c
if (pos > 4096 || n > 4096 - pos)
    return -1;                 /* nothing copied */
memcpy(area + pos, src, n);
```
