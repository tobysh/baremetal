---
title: Verify a signed token safely
summary: Check a JWT-like token's structure, pin the algorithm, and compare the signature over the exact received bytes.
minutes: 40
difficulty: medium
tags: security, auth, tokens
id: sec-appsec/token-signature
mode: stdout
---
A token has three dot-separated parts: a header, a payload and a signature, for example `aGVhZGVy.cGF5bG9hZA.9f2c...`. The header and payload are base64url text of `key=value;key=value` data, and the signature is a MAC over the first two parts exactly as received. In this course the MAC is a toy (FNV based, shown in the starter as `toy_mac_hex`) so you can write it by hand. It is not secure; real systems use HMAC-SHA-256 from a vetted library.

The most famous token bug is trusting the header. If a verifier lets the token choose the algorithm, an attacker writes `alg=none` and drops the signature, or flips an algorithm so that a public value is used as a MAC key. Pin the algorithm on the server: only `HS256` exactly (case and spelling matter) is accepted. Be equally careful with the other classics: compute the MAC over the received text of the first two parts (never over a re-encoded version), compare all 16 characters without stopping at the first difference as in [constant-time comparison](/learn/sec-crypto/constant-time), and treat duplicate header keys as malformed, because two parsers may pick different copies.

Decoding comes from [the previous lesson](/learn/sec-appsec/base64url); it is provided in the starter.
