Implement the arena API on top of this struct (already in the starter):

```c
typedef struct { unsigned char *base; size_t cap; size_t off; } Arena;
```

- `void arena_init(Arena *a, void *buf, size_t cap)`: use `buf` (any alignment) as storage; nothing is allocated yet.
- `void *arena_alloc(Arena *a, size_t size, size_t align)`: the returned **address** is a multiple of `align`, and the block lies fully inside the buffer. Padding bytes count as used. Return `NULL` and change nothing if `size == 0`, `align` is not a power of two, or there is not enough room (including when sizes are near `SIZE_MAX`).
- `size_t arena_used(const Arena *a)`: bytes consumed so far, padding included.
- `size_t arena_mark(const Arena *a)` and `void arena_rewind(Arena *a, size_t mark)`: rewinding to a `mark` greater than the current offset does nothing.
- `void arena_reset(Arena *a)`: release everything.

You may paste your `align_up` and `padding_for` from the previous lesson, but they are not provided. Hidden tests run seeded random sequences of allocate / mark / rewind / reset against a model that tracks every address, and use buffers that start at misaligned addresses.
