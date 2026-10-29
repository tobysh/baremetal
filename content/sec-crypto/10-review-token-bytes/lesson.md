---
title: Review: token bytes
summary: Combine strict hex decoding and a complete byte comparison while keeping encoding separate from authentication.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-crypto/review-token-bytes
mode: harness
kind: review
---
This review combines [hex encoding](/learn/sec-crypto/hex) with [full-scan equality](/learn/sec-crypto/constant-time). A presented token is lowercase or uppercase hexadecimal text. The expected token is a byte span. Decode exactly two digits per expected byte, reject invalid text, and compare the decoded bytes.

Encoding changes representation; it does not add secrecy or authenticity. Comparing an unpredictable secret token can be one component of authentication, but this toy function generates no secrets and does not implement session storage, replay protection or rate limits. Those concerns remain separate.

Length and malformed-input checks can return early because the syntax and expected length are public in this contract. Once valid bytes are available, accumulate XOR differences across the entire expected length. Check `n <= SIZE_MAX/2` before calculating a doubled text length. The helper accepts empty tokens for the exercise; an application should impose its own nonempty fixed-length policy. No new cryptographic primitive is introduced here.
