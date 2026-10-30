---
title: Dictionary attacks and why slow hashes win
summary: Crack synthetic toy hashes with a word list, measure the work, see how salts and iteration counts change the cost, and size an iteration count.
minutes: 35
difficulty: medium
tags: passwords, hashing, brute-force, mitigation
id: ethical-hacking/dictionary-attack
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the 'password database' is a handful of numbers made by a toy hash, with a word list of invented words. The hash is deliberately weak and is not a real algorithm. In scope: those numbers. No account, service or real hash is attacked. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

Websites should never store passwords; they store a **hash**, a one-way fingerprint. If the database leaks, the attacker has fingerprints, not passwords, but people choose guessable passwords, so the attacker simply hashes a long **word list** and sees which fingerprints match. That is a **dictionary attack**, and it is *offline*: no login page, no [rate limit](/learn/ethical-hacking/rate-limiting), no lockout can slow it. The only things that slow it are properties of the stored hash.

The toy hash here is `toy_hash(salt, password, iters)`: a simple mixing function (FNV-1a over the salt then the password), then repeated `iters - 1` more times over its own output. It is **not** secure, and exists so that every round is countable.

Three defences, each measured by **work** (hash rounds the attacker must compute):

- **A salt** is a random value stored next to the hash and mixed in. Without salts, identical passwords give identical hashes, so one pass over the word list cracks every user at once, and the database reveals who reuses a password. With a per-user salt, each user must be attacked separately and shared passwords are hidden.
- **Many iterations** (a *work factor*) multiply the cost of every guess, for the attacker and defender alike. The defender pays it once per login. The attacker pays it for every guess, and billions of guesses make the difference between minutes and centuries.
- **A purpose-built password hash** (scrypt, Argon2, bcrypt, PBKDF2) does this correctly, with memory cost, and has been reviewed by experts. Never invent your own; this lesson only measures *why*.

## Your tasks

Attack one salted hash and count the work. Then switch to the defender: spot password reuse in an unsalted database, and compute the iteration count that makes a full word-list pass take a target time.
