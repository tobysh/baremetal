---
title: Interrupted Updates and Recovery
summary: Crash a save at every step. Write to a temp file and rename it into place, then make loading validate the file and clean up leftovers.
minutes: 40
difficulty: hard
tags: crash-consistency, atomic-rename, recovery
id: go-fault-injection/atomic-update
mode: harness
---
Overwriting a file in place is not atomic. If the process dies halfway, the file holds a **torn** mix of old and new bytes, and the old version is gone. A saved file must always be one of: the old version, or the new version, never a blend.

## Write-then-rename

The standard fix uses the one step a file system makes all-or-nothing, rename:

1. Write the complete new content to a temporary file (`db.tmp`).
2. Rename `db.tmp` over `db`. Readers see the old `db` or the new `db`.

A crash during step 1 leaves a torn `db.tmp` and an intact `db`. A crash during step 2 leaves either result. On startup, `Load` ignores and deletes any stray `db.tmp`.

## The checksum envelope

Files are stored as `len (u32) | data | crc32 (u32)`; `encode` and `decode` are given. `Load` still validates, because a file can be damaged for reasons other than your own crash. A file that fails validation is reported as `ErrCorrupt`, never returned as data.

## The fake disk

The hidden tests use an in-memory `Disk` that counts operations (`WriteFile`, `Rename`, `Remove`) and **crashes at operation `k`**, for every `k`. At the crash point:

- a `WriteFile` stores only a prefix of the data (0 bytes, 1 byte, half, or all but one) and returns `ErrCrash`,
- a `Rename` or `Remove` either did not happen, or did happen but the reply was lost,
- every later operation fails until `Reboot`.

After each crash the test reboots, calls your `Load`, and checks the invariants: the result is not corrupt, includes every version whose `Save` returned `nil` (the **committed** data), and is either the last committed version or the one that was in flight. Then it saves again to check that recovery left the disk usable.

## Limits of this model

The model assumes `Rename` is atomic and that `WriteFile` data survives a crash up to the torn prefix. Real systems also need `fsync` on the file and its directory, and some file systems reorder writes. This lesson teaches the structure of crash-safe updates; it does not prove durability on any real disk.
