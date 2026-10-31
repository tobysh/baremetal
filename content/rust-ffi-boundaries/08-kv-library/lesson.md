---
title: "Capstone: A C-ABI Key-Value Library"
summary: Combine layout, callbacks, owned handles and error codes into one small library with a C-style interface, checked against a model.
minutes: 45
difficulty: hard
tags: ffi, capstone, handles, callbacks
id: rust-ffi-boundaries/kv-library
mode: harness
---
Everything from this track in one API, the way a C header would declare it:

```c
Store *kv_new(void);
void   kv_free(Store *);
int    kv_add(Store *, const uint8_t *key, size_t key_len, int64_t delta, int64_t *out);
int    kv_get(const Store *, const uint8_t *key, size_t key_len, int64_t *out);
int    kv_remove(Store *, const uint8_t *key, size_t key_len);
ssize_t kv_each(const Store *, int (*cb)(void *ctx, const Entry *), void *ctx);
```

- The store is an opaque handle made with `Box::into_raw` (lesson 5).
- `Entry` is `#[repr(C)]`: a key pointer, a key length and a value (lesson 1). The key pointer is only valid during the callback.
- `kv_each` takes a callback and a context, visits keys in ascending byte order and stops when the callback returns non-zero (lesson 3).
- Every function returns a status code, never panics, and leaves `out` untouched on error (lesson 6).
- Keys are `(ptr, len)` byte strings (lesson 4).

## Honest limit

The tests compare your library with a model (a `BTreeMap`) over long pseudo-random operation sequences, count live stores to catch leaks, and check `Entry`'s size. They cannot detect use after free or a dangling key pointer kept past a callback.
