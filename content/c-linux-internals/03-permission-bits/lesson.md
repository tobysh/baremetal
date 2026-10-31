---
title: Mode Bits and Credentials
summary: Decide read, write or execute from mode bits, owner, group and effective ids, including the owner-class trap and root's exceptions.
minutes: 30
difficulty: medium
tags: permissions, uid-gid, kernel-model
id: c-linux-internals/permission-bits
mode: harness
---
Every file has an owner, a group and nine permission bits, written like `rw-r-----` (octal `0640`): three bits each for **owner**, **group** and **other**.

```c
typedef struct { int ino; unsigned uid, gid; unsigned mode; int is_dir; } inode;
typedef struct { unsigned uid, euid, gid, egid; unsigned groups[4]; int ngroups; } cred;
```

## The rule

The kernel picks **one class** and looks only at its bits:

1. If the caller's **effective** uid equals the owner, use the owner bits.
2. Otherwise, if the file's group is the caller's effective gid or one of its supplementary groups, use the group bits.
3. Otherwise use the other bits.

Two traps follow. A file with mode `0070` is unreadable by its owner (the owner class is chosen and has no bits), even though the group could read it. And the *real* uid is irrelevant: only the effective ids count.

**Root** (effective uid 0) skips the read and write bits, but execute is still refused on a regular file that has no execute bit for anyone. Directories can always be searched by root.

## Model limits

This models classic mode bits only. ACLs, capabilities, mount options, `umask`, immutable flags and security modules (SELinux) also decide access on a real system and are not represented. The `ino` field is carried along for later lessons.

## Your turn

Implement `may_access(n, c, want)`, where `want` is an OR of `P_R` (4), `P_W` (2) and `P_X` (1); all requested bits must be allowed. The hidden tests contain hand-written cases and an exhaustive sweep of every mode against a separately written reference. A justification of *why* a class choice is made is not machine-graded; try to explain the `0070` trap in your own words.
