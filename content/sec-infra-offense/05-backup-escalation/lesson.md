---
title: Escalate through writable root-job configuration
summary: Change a fictional root job, verify elevated access and restore both the configuration and session privilege.
minutes: 40
difficulty: hard
tags: security, offensive-lab, backup-escalation
id: sec-infra-offense/backup-escalation
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

Linux privilege escalation often follows a trust path: a root-run job consumes a file writable by a lower-privilege user. Permissions matter because they determine who can influence that job. In this lab the backup configuration is a synthetic file with a root owner, a Writable flag and an inert command field.

The tiny job language has `archive` (normal behavior) and `grant-root` (sets the current toy session's role to root). It is not shell syntax and never changes the runner's identity. Merely seeing writable metadata is not exploitation; the root-only marker must become readable. Restore the configuration and drop the elevated toy role afterward, verifying both by fresh reads. Cleanup failures must not be reported as a fully successful exercise.
