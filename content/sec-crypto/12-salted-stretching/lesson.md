---
title: Salt and stretch a password
summary: Implement a toy salted, iterated password hash to see why salts defeat shared tables and iterations slow guessing. Use real KDFs in practice.
minutes: 30
difficulty: medium
tags: security, crypto-misuse, passwords
id: sec-crypto/salted-stretching
mode: harness
---
Storing passwords as plain text is a disaster when the database leaks, and storing one fast hash of each password is only a little better. If two users share a password they share a hash, and an attacker can precompute hashes of common passwords once and look up every account. A per-user random salt, stored beside the hash, makes equal passwords produce different hashes and forces the attacker to work on each account separately.

Guessing speed matters as well. An iteration count makes each guess cost many hash steps, so an attacker doing billions of guesses pays for every one, while a login that checks one password barely notices. Real systems use a purpose-built password hash such as scrypt, Argon2 or PBKDF2 from a vetted library, with parameters tuned to the hardware and stored with the result, so they can be raised later.

This exercise uses a deliberately weak toy built from the noncryptographic 64-bit FNV-1a hash, so that you can see the structure with nothing else in the way. Start the state from the salt then the password; then for each iteration hash the 8 bytes of the state (least significant byte first) followed by the salt again. It must never protect a real secret. The password and salts here are made up.
