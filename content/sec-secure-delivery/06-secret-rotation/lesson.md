---
title: Rotate a secret without an outage
summary: Keep several secret versions with a grace period, revoke a leaked one at once, refuse reused or weak secrets and clock regressions, all under a fake clock.
minutes: 40
difficulty: hard
tags: secrets, rotation, revocation, clocks
id: sec-secure-delivery/secret-rotation
mode: harness
---
When the scanner from the last lesson finds a leaked secret, the fix is to make the old value **useless**. Two forces pull against each other:

- **Rotation** replaces a secret on a schedule or after an incident. If the old value stops working the instant the new one exists, every client still holding the old one breaks. So the old version stays valid for a **grace period** while clients catch up.
- **Revocation** is the emergency button: a leaked version must stop working *now*, grace period or not.

## The model

A `KeyRing` holds numbered **versions** (1, 2, 3, ...). Time is an `int64` you are given, never the real clock.

- `Rotate(secret, now)` adds a new current version. The previous current version stays valid until `now + grace` (exclusive), unless it was revoked, or already had an earlier end. Older versions keep whatever end they already had.
  - A secret shorter than 16 bytes is `ErrWeak`.
  - A secret equal to **any** earlier version, revoked or expired, is `ErrReuse`. Re-adopting a leaked value defeats the point.
  - `now` earlier than the previous rotation time is `ErrClock`: a clock that moved backwards must not extend any version's life.
  - On any error the ring is unchanged.
- `Revoke(version)` makes that version invalid immediately and permanently (`ErrNoSuchVersion` if it was never issued). Revoking the current version leaves no valid current secret until the next `Rotate`.
- `Check(secret, now)` returns `(version, true)` if the secret equals a version that is not revoked and has not ended (`now < end`). The current version never ends by time. If somehow several versions match (they cannot, because reuse is refused) return the highest.
- `Current()` returns the number of the latest version, or 0.

## Constant-time comparison, no early exit

Compare with `subtle.ConstantTimeCompare` against **every** version, and do not return at the first hit, so the time taken does not reveal which version matched or how many bytes were right. Tests cannot measure timing, so this is a habit to apply and review rather than something graded here.

## Limit

This models verifying shared secrets at one server. It does not model distribution to clients, storage, or how you decide a secret leaked. Stored secrets in a real system should be hashed or held by a vault; the ring keeps them in memory only because the lesson needs to compare them.
