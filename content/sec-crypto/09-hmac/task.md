The starter includes a working SHA-256 with a streaming interface: `sha256_init`, `sha256_update` and `sha256_final`. Do not change it. The two functions below are the exercise. The starter's versions use the broken `sha256(key || msg)` construction.

1. **`void hmac_sha256(const uint8_t *key, size_t klen, const uint8_t *msg, size_t mlen, uint8_t out[32])`** writes exactly 32 bytes. Follow RFC 2104 with a 64-byte block: hash a key longer than 64 bytes first, zero-pad the key to 64 bytes, XOR with `0x36` for the inner hash and `0x5c` for the outer hash. A key may be empty, and a message may be empty (the pointers may then be anything).
2. **`int hmac_verify(const uint8_t *key, size_t klen, const uint8_t *msg, size_t mlen, const uint8_t *tag, size_t taglen)`** returns 1 only if `taglen` is 32 and the tag equals the HMAC of the message. Compare all 32 bytes with no early exit.

Your output is checked against published and generated test vectors with keys and messages of many lengths, including keys of 0, 64, 65 and 131 bytes.
