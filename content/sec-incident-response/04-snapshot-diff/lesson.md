---
title: Diff filesystem snapshots
summary: Compare a baseline and a current file manifest, set aside expected updates, and flag timestamp tampering without chasing noise.
minutes: 40
difficulty: hard
tags: incident-response, filesystem, anti-forensics
id: sec-incident-response/snapshot-diff
mode: harness
---

A baseline manifest (path, content hash, mode, owner, modification and change times) is the cheapest way to find what an intruder changed. The diff has three jobs: report real changes, ignore harmless ones, and notice when the evidence has been tampered with.

**Noise.** A file whose timestamps moved but whose hash, mode and owner did not is not a content change. Package updates change files legitimately, so a short allowlist maps a path to the exact hash a trusted update produces. Matching the allowlisted hash makes a change expected; any other hash on that path does not.

**Timestamps.** Setting a file's modification time (mtime) back is easy for someone with write access. The change time (ctime) is updated by the kernel whenever the file or its metadata changes, and ordinary tools cannot set it. Two cheap checks follow: a file whose mtime is later than its ctime is impossible without tampering, and a file whose content hash changed but whose mtime equals the baseline's mtime was probably restored to look untouched. These are indicators, not proof, and an attacker with more access (clock changes, raw disk writes) can defeat them.

In this model all times are Unix seconds from a trusted clock, and the manifest itself is assumed trustworthy.
