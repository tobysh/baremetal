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

## How the pieces map

- The store is an opaque handle made with `Box::into_raw` and freed once with `Box::from_raw` (lesson 5). `kv_free(NULL)` is a no-op.
- `Entry` is `#[repr(C)]`: a key pointer, a key length and a value, so a C caller can read it (lesson 1). The key pointer borrows from the store and is only valid during the callback; a caller that wants the key later must copy it.
- `kv_each` takes a callback and a context, visits keys in ascending byte order and stops when the callback returns non-zero (lesson 3).
- Every function returns a status code, never panics, and leaves `out` untouched on error (lesson 6).
- Keys are `(ptr, len)` byte strings, never assumed to be UTF-8 or NUL-terminated (lesson 4).

```text
caller                    library
kv_new() ---------------> Box<Store> -> raw ptr (ownership moves out)
kv_add(s, key, n, d, &v)  validate: s? n==0? key? -> checked_add -> insert -> *out = v
kv_each(s, cb, ctx) ----> for (k, v) in map: Entry{k.as_ptr(), k.len(), v} -> cb(ctx, &e)
kv_free(s) -------------> Box::from_raw(s) dropped once
```

## Design decisions to make deliberately

Pick the order of argument checks and keep it identical in every function, so a helper can do them once. Decide that a failed `kv_add` changes nothing at all: compute the new value with `checked_add` first and mutate the map only on success, otherwise a missing key would be created by a call that reported failure. Make `out` optional (null allowed) when the caller may not care, but never write it on an error. Remember that `Entry` is built on the stack per iteration, so the pointer to it is also only valid during that call.

## Pitfalls

- Holding a `&mut Store` across a callback that might call back into the store. This API forbids it: `kv_each` takes a shared `*const Store`.
- Creating a `&[u8]` from `(ptr, len)` before checking null.
- Forgetting that ordering is by bytes, so `"app" < "apple"` and `"\x00\xff"` sorts first.

## The SAFETY convention

Every `unsafe` block carries a `// SAFETY:` comment, and every `unsafe fn` a `# Safety` section naming what the C caller must uphold: a live handle from `kv_new`, a key valid for `key_len` bytes, an `out` that is null or writable. A small private helper that validates arguments and returns a safe `&[u8]` keeps the unsafe surface in one place.

## Honest limit

The tests compare your library with a model (a `BTreeMap`) over long pseudo-random operation sequences, count live stores to catch leaks, and check `Entry`'s size. They cannot detect use after free or a dangling key pointer kept past a callback.
