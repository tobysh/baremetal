---
title: Rate limiting
summary: Build a sliding-window rate limiter that slows scans and guessing, return a retry time, and decide what to do when the table fills up.
minutes: 30
difficulty: medium
tags: mitigation, rate-limiting, brute-force
id: ethical-hacking/rate-limiting
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the limiter is a plain data structure in your own code and the tests feed it made-up keys and clock values. In scope: that structure. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

[Detecting a scan](/learn/ethical-hacking/detect-the-scan) tells you it happened. A **rate limiter** makes the attack slow enough not to matter. Guessing a password or probing a service works by volume, so capping how often one client may try shrinks what the attacker can do per hour.

A **sliding window** limiter allows at most `limit` requests per key in any span of `window` seconds. Each key (an address, an account name) keeps the timestamps of its recent allowed requests. A new request at time `now` is allowed if fewer than `limit` timestamps are newer than `now - window`.

Design decisions a defender has to make, and your tests check:

- **Blocked requests are not recorded.** If they were, a client who keeps hammering would never recover.
- **Say when to come back.** `rl_retry_after` returns how many seconds until the oldest in-window request expires. That is the `Retry-After` of an HTTP 429 response.
- **Bounded memory.** The table holds a few keys. A key whose requests have all expired is free to reuse. When the table is full of *live* keys a new key is **refused** (fail closed). Real systems also have to think about whether failing closed lets an attacker lock others out, which is a good question to carry into the [login lesson](/learn/ethical-hacking/dictionary-attack).
