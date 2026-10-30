---
title: Compare every byte
summary: Replace an early-exit equality check with a full scan and understand what source-level timing discipline can prove.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-crypto/constant-time
mode: harness
---
An ordinary string comparison stops at the first mismatch. For a secret token, repeated measurements can reveal how much of a guessed prefix matched. A useful first repair is to examine every byte and accumulate differences without branching on their values.

Compute `diff |= a[i] ^ b[i]` for every index, then compare diff to zero once. There is no need to count the differing bytes: any nonzero accumulated difference means the inputs differ. Inputs here have a public, fixed length; branching on that length or on invalid pointers is outside the secret-dependent scan.

The exercise exposes an optional `visited` output so tests can check the intended loop work. This is an educational accounting aid, not a timing measurement or proof. Compilers, machine instructions, caches and the surrounding protocol can all affect timing. In production use the vetted library's constant-time comparison function and its length contract. Do not invent a delay, random sleep or hand-tuned loop and call it secure. Constant-time discipline concerns the whole path that handles the secret.
