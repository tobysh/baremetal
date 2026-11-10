Treat `used` as 64 independent yes/no answers. Every operation is one of three bit operations on bit `i`: test it with `used & (1ULL << i)`, set it with `|=`, clear it with `&= ~`. The work is in deciding *which* `i` is safe to touch.
===
In `frame_reserve`, the dangerous expression is `first + count`. Both are `unsigned`, so the sum can wrap around to a small number and your loop runs zero times. Decide whether `first` is inside the pool before doing any arithmetic with `count`.
===
Once `first < 64` is known, `64 - first` cannot underflow, and comparing `count > 64 - first` tells you whether the range runs off the end. Shrink `count` to fit; after that, `first + count <= 64` and the loop bound is safe.
===
For `frame_release`, check in the order that keeps every later step well defined. A similar pattern on a different structure:

```c
/* slot table of 16 entries, 32 bytes each, starting at TABLE */
if (p < TABLE || p >= TABLE + 16 * 32) return -1;   /* range first */
if ((p - TABLE) % 32) return -1;                    /* then alignment */
unsigned i = (p - TABLE) / 32;                      /* only now an index */
if (!in_use[i]) return -1;                          /* then state */
```
