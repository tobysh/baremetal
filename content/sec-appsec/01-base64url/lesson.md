---
title: Decode base64url strictly
summary: Write a strict unpadded base64url decoder, the encoding tokens use, and reject every non-canonical form.
minutes: 35
difficulty: medium
tags: security, auth, encoding
id: sec-appsec/base64url
mode: harness
---
Web tokens are text so they travel in headers and URLs, and the binary parts are written in base64url: the base64 alphabet with `-` and `_` instead of `+` and `/`, and no `=` padding. You wrote ordinary base64 in the [crypto course](/learn/sec-crypto/base64); now tighten it for security.

A lenient decoder is a risk. If one decoder skips whitespace or accepts `+` and `/` and the other does not, the same token can mean different things to two systems, which is how request smuggling and signature bypasses start. Be strict: only the 64 characters of the alphabet, no padding, no whitespace, no NUL. A length that is 1 modulo 4 cannot be valid, because one character carries only six bits. Finally require canonical form: after the last full byte, the leftover bits must all be zero. `QQ` decodes to `A`, but `QR` has stray bits set and encodes the same byte another way. Accepting both gives an attacker two spellings of the same token, and any check on the text of the token (blocklists, cache keys, signature inputs) then disagrees with the check on its meaning.

Also protect the output buffer: write into `out` only while `o < cap` and fail otherwise, in the style of [bounded copying](/learn/sec-c-bugs/unbounded-copy).
