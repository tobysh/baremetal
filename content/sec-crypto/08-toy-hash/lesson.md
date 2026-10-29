---
title: Collisions in a tiny hash
summary: Find two distinct two-byte messages with the same deliberately tiny FNV-derived hash.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-crypto/toy-hash
mode: harness
---
A hash maps arbitrary messages into a fixed output space. With only 256 outputs, 257 distinct inputs guarantee a collision by the pigeonhole principle. This lesson deliberately keeps only the low eight bits of FNV-1a so you can see the entire state space.

Start at `0x811c9dc5`, XOR each byte, multiply by `0x01000193` modulo 2^32, and finally keep `h & 255`. Use `uint32_t` so wraparound is defined. FNV is a noncryptographic hash even before this truncation: it is not a password hash, MAC, signature or integrity proof against an attacker.

To find a collision, enumerate every two-byte message in increasing little-endian integer order: value v represents `{v & 255, v >> 8}`. Remember the first v for each hash bucket. On the first occupied bucket, return the earlier and current values. The deterministic order makes the exercise reproducible; it is not a claim about breaking a modern cryptographic hash. Even a strong digest requires an authenticated reference to establish who created it, and plain hashes alone do not authenticate messages.
