---
title: Spot repeated blocks
summary: Identify equal full blocks as evidence of ECB-like pattern leakage without implementing an encryption algorithm.
minutes: 25
difficulty: easy
tags: security, defensive-programming
id: sec-crypto/detect-ecb
mode: harness
---
A block cipher transforms a fixed-size block under a key. In Electronic Codebook mode, equal plaintext blocks under the same key produce equal ciphertext blocks. This exposes structure even when individual bytes look random. Detection needs only byte comparison, not the key.

Split the input into full blocks of `block_size` bytes and compare every pair. A trailing partial block is ignored because it is not a complete cipher block. Return whether any pair matches. Do not compare only adjacent blocks: repetitions may be far apart. A quadratic scan is fine for these small fixtures and needs no allocations.

Repeated ciphertext blocks are evidence, not proof of a particular mode. Duplicated records, an unusual protocol or chance can also cause repetition. Conversely, a short ECB message may contain no repeats. The exercise accepts arbitrary block sizes to teach the reasoning; AES uses 16-byte blocks. No actual encryption occurs here. Real systems normally use a vetted authenticated-encryption construction with its required nonce policy, rather than selecting a mode from a toy detector.
