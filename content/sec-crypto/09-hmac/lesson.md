---
title: HMAC from a hash
summary: Build HMAC-SHA-256 from a supplied hash, check it against known test vectors, and see why hashing key followed by message is not a MAC.
minutes: 45
difficulty: hard
tags: crypto, hmac, mac, length-extension
id: sec-crypto/hmac
mode: harness
---
A **message authentication code** (MAC) lets two parties who share a secret key check that a message came from the other one and was not changed. The obvious recipe is `sha256(key || message)`: hash the secret, then the message. It is wrong.

SHA-256 is built as a chain. It reads the input in 64-byte blocks and carries a 32-byte internal state from one block to the next, and the digest *is* that final state. So anyone who sees `sha256(key || message)` already holds the hash's internal state after `key || message`. Without knowing the key they can load that state and keep hashing: the result is a valid hash of `key || message || padding || more`, for data of their choosing. This is a **length-extension** weakness. It is the reason the secret-prefix MAC is broken, and it affects MD5, SHA-1 and the SHA-2 family (not SHA-3). Putting the key at the end (`hash(message || key)`) fails differently: any collision in the hash then becomes a collision for every key.

**HMAC** (RFC 2104) avoids both problems by hashing twice with two different key-derived pads:

```
HMAC(K, m) = H( (K' xor opad) || H( (K' xor ipad) || m ) )
```

where `K'` is the key padded with zero bytes to the 64-byte block size (a key longer than 64 bytes is hashed first, and the 32-byte digest is padded), `ipad` is the byte `0x36` repeated 64 times and `opad` is `0x5c` repeated 64 times. The outer hash covers a fixed-length input made of the outer pad and the inner digest, so there is nothing left to extend.

Checking a tag has its own rule: compare all 32 bytes without stopping at the first difference, as in [constant-time comparison](/learn/sec-crypto/constant-time), and refuse a tag of the wrong length. The earlier [toy hash](/learn/sec-crypto/toy-hash) lesson showed what a tiny hash gives away; this one builds the real thing. Everything here is for learning. In real software use a vetted library.
