**Challenge: derive keys with HKDF.**

Write `int hkdf_sha256(const uint8_t *salt, size_t slen, const uint8_t *ikm, size_t ilen, const uint8_t *info, size_t infolen, uint8_t *out, size_t olen)`, the key derivation function of RFC 5869, on top of the `hmac_sha256` from the lesson (given in the starter).

- **Extract.** `prk = HMAC(salt, ikm)`. If `salt` is `NULL` or `slen` is 0, use 32 zero bytes as the salt.
- **Expand.** `T(1) = HMAC(prk, info || 0x01)`, `T(2) = HMAC(prk, T(1) || info || 0x02)`, and so on. Concatenate the blocks and write exactly the first `olen` bytes to `out`.
- **Be defensive.** The counter is one byte, so at most 255 blocks (8160 bytes) can be produced. For a larger `olen` return -1 and write nothing. On success return 0. `olen` may be 0, `info` may be empty (the pointer may then be anything), and you must never write past `out[olen - 1]`.

Expected outputs come from the RFC 5869 vectors and from Python's `hmac` module, with salts, inputs and contexts of many lengths.
