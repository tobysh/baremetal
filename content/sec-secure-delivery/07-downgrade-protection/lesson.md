---
title: Refuse replayed, stale and downgraded updates
summary: Decide whether a correctly signed update offer should be accepted, using a monotonic counter, a high-water mark, freshness and expiry against a fake clock.
minutes: 45
difficulty: hard
tags: updates, rollback-protection, replay, freshness
id: sec-secure-delivery/downgrade-protection
mode: harness
---
A valid signature says "the publisher signed this once". It does not say "this is the newest thing the publisher signed". Old releases stay validly signed forever, including the ones with known vulnerabilities. An attacker who can only *serve* files (a mirror, a proxy, a stolen CDN credential) cannot forge signatures, but can offer you an **old signed update** and have you install it. That is a **downgrade** (rollback) attack. Related attacks:

- **Replay**: re-offering something you already have, to waste work or hide that a newer version exists.
- **Freeze**: serving old signed metadata forever so you never learn of an update. Defence: signed metadata expires and its issue time may never go backwards.
- **Equivocation**: the same version number with different content.

Signature checking (lessons 2 and 3) happens first. This lesson decides what to do with an offer that already verified.

## State and offer

```go
type State struct { Counter, HighWater uint64; Digest string; LastIssued int64 }
type Offer struct { Version string; Counter uint64; IssuedAt, ExpiresAt int64; Digest string }
```

- `Counter` is a monotonic release number chosen by the publisher. Do not compare the human `Version` strings; `1.10` versus `1.9` is a classic bug, and a signed version label can be wrong.
- `Counter` in `State` is what is **running**. `HighWater` is the highest counter ever committed on this device. After a local rollback to a good version the running counter can be *lower* than `HighWater`, but the broken one must not be re-offered and accepted.
- `LastIssued` is the newest `IssuedAt` seen so far.

## Rules, in this order

Time is the argument `now`; `Skew` (300 ticks) allows for clock error.

1. `ErrMalformed`: empty `Version` or `Digest`, `Counter == 0` (counters start at 1), or `ExpiresAt <= IssuedAt`.
2. `ErrFuture`: `IssuedAt > now + Skew`.
3. `ErrExpired`: `now >= ExpiresAt`.
4. `ErrStaleMetadata`: `IssuedAt < State.LastIssued`.
5. Counter equals `State.Counter`: `ErrAlreadyInstalled` if the digest matches, otherwise `ErrEquivocation`.
6. `ErrDowngrade`: `Counter <= State.HighWater`, or `Counter < State.Counter`.
7. Otherwise accept (return nil).

`Apply(st, offer)` returns the state after a successful install: counter, digest, `HighWater = max(HighWater, Counter)` and `LastIssued = max(LastIssued, IssuedAt)`.

## What this does not defend

The decision trusts `now`. An attacker who controls the device clock can make expired offers look valid and push `now` forward to expire good ones; real clients keep a trusted time source or at least refuse to let time go backwards. Counters protect against replay only on a device that remembers its state: a factory reset or a restored backup resets `HighWater`, which is why the next lesson stores it carefully.
