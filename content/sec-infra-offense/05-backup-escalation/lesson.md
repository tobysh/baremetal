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


## The model

```text
low-privilege user ──write──▶ /backup/config   (owner root, Writable true, Command "archive")
                                   │
root-run job reads it ◀── POST /backup/run
                                   │
        Command "archive"    -> normal backup, nothing changes
        Command "grant-root" -> session role becomes root
                                   │
                         GET /root/proof -> 200 "LAB-PROOF:..."   <- the evidence
```

Writable metadata is only a *lead*. The finding is the root-only marker becoming readable, and the work is not finished until the state is back where you found it.

## A worked example

Starting state: role `user`, config `{Path:"/opt/job-3", Owner:"root", Writable:true, Command:"archive"}`.

```text
GET  /identity        -> 200 "user"
GET  /backup/config   -> 200 {... Command:"archive"}      keep this exact copy
PUT  /backup/config   {... Command:"grant-root"}          -> 200   (only Command changed)
POST /backup/run                                          -> 202
GET  /root/proof      -> 200 "LAB-PROOF:root-3"           impact confirmed
PUT  /backup/config   <the saved original>                -> 200   cleanup starts here
POST /session/drop-root
GET  /backup/config   -> must equal the saved original
GET  /identity        -> must be "user"
```

Notice that only the `Command` field changes in the `PUT`: the path, owner and flag must match the original, or the model treats it as a different edit. In Go, decode the original into a `BackupConfig` value, copy it, change one field and marshal the copy. Compare the fresh read with `==`.

## Pitfalls

- Treating `202` from `/backup/run` as success. The run status says the job ran; only `/root/proof` says you became root.
- Returning early when the proof fails. If the write was accepted, state has changed and cleanup is mandatory. The tests make `/root/proof` return `503` to check this.
- Trusting the cleanup replies. A server can answer `200` to the restoring `PUT` without restoring anything; only the fresh `GET`s count.
- Writing before checking. Owner, writable flag and command must all match before you edit anything.

## The defender's half

The root job should never read configuration a lower-privilege user can write: make the file root-owned and not group- or world-writable, and have the job validate its own settings against an allow-list of commands. Alert on changes to files that privileged jobs consume. Real systems add wrinkles the toy leaves out, such as writable parent directories, symlinks and scheduled-job environments, but the trust path is the same.
