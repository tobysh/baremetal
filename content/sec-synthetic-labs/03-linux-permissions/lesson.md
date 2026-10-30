---
title: Linux permissions from an archived filesystem
summary: Inspect a synthetic tar archive for world-writable files, root SUID programs and exposed secrets.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, linux-permissions
id: sec-synthetic-labs/linux-permissions
mode: harness
---

Linux permission bits distinguish owner, group and everyone else. Other-write is 0002, other-read is 0004 and SUID is 04000. A root-owned executable with SUID deserves review, but SUID alone does not prove an exploitable bug. A world-writable directory has different semantics from a file; this exercise reports regular files only.

The supplied tar archive represents a fictional filesystem. Read header metadata without extracting anything or changing host permissions. Paths in the archive are relative. Inspect regular files, skip directories and symbolic links, and reject corrupt archives. This works without mounting a filesystem or granting elevated privileges.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.
