---
title: Decide which key may sign what
summary: Look keys up in a trusted store, enforce roles, validity windows and revocation with an explicit clock, and accept key rotation only from a root key.
minutes: 40
difficulty: medium
tags: supply-chain, trust-boundary, key-rotation, revocation
id: sec-secure-delivery/key-trust
mode: harness
---
A valid signature only says "some private key signed this". Whether to *act* on it depends on questions the signature cannot answer:

- Is this key one **we** chose to trust? The envelope names a key id, but that name is attacker-controlled text. Look the id up in a **trust store** you hold; never take the public key from the message.
- Is the key allowed to sign *this kind* of thing? A key that signs releases must not be able to add new trusted keys, or compromising it would compromise the whole store. Give each key a **role** (`root` or `release`).
- Is it valid *now*? Keys have a validity window `[NotBefore, NotAfter)`. Take time as a parameter (an `int64` fake clock) so behaviour is deterministic.
- Has it been **revoked**? A revoked key is rejected even though its signature is mathematically fine and its window is open.

## Domain separation

The signature covers `SigningInput(keyID, payload)`, supplied in the starter: a fixed label, the key id and the payload. The label stops a signature made for one purpose from being replayed as another, and including the key id stops an attacker relabelling a signed payload as coming from a different key.

## Check order

`Verify` checks, in this order: unknown key (`ErrUnknownKey`), revoked (`ErrRevoked`), wrong role (`ErrWrongRole`), outside the window (`ErrKeyExpired`), then the signature (`ErrBadSignature`). Cheap policy checks run before the cryptography, and an unauthorised key never gets to be "verified".

## Rotation

Keys must be replaceable. `ApplyRotation` takes a `Signed` message that must verify with role `root`. Its payload is a list of lines, each `add <id> <role> <notBefore> <notAfter> <pubkey-hex>` or `revoke <id>`. It is **all or nothing**: if any line is malformed (unknown verb, bad role, bad number, bad key length, duplicate id, revoking an unknown id), the store is unchanged. Adding an id that already exists is `ErrDuplicateKey`.

Limit of this model: one compromised root key still compromises everything. Real systems require several root signatures (a threshold) and keep root keys offline; the exercise stops at a single signer to stay readable.
