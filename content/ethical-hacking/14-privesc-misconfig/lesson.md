---
title: Privilege escalation misconfigurations
summary: Audit a synthetic file-permission listing for places where an unprivileged user can change what root runs, and propose the minimal fix for each.
minutes: 40
difficulty: medium
tags: privilege-escalation, permissions, hardening, audit
id: ethical-hacking/privesc-misconfig
mode: stdout
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the listing on stdin is a made-up description of files, services and one pretend low-privilege user named guest. In scope: the text you read. You change no real permissions. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

Most intrusions start with a small foothold: a stolen low-privilege account, a bug in one application. The attacker's next goal is **privilege escalation**, becoming a more powerful user, usually root. Often no clever bug is needed, just a **misconfiguration**: something powerful that a weak user is allowed to change.

The recurring pattern is simple. If root runs a program or script, and an ordinary user can **modify that file**, the user can make root run anything. The same is true if the user can **replace the file** because they can write to the *directory* it sits in. Three classic shapes:

- a service or scheduled job runs as root from a file writable by others,
- the same file sits in a directory writable by others (so it can be swapped even if the file itself is locked),
- a **setuid** program (mode bit `04000`, runs with its owner's rights) that is itself writable.

Your task is the defender's audit. You read a listing and report each path with the smallest fix. This is the same skill as in [persistence](/learn/ethical-hacking/persistence), where root-run entries in writable places were the worst.

## Input

```text
G guest staff            guest belongs to group staff (any number of G lines)
F 0755 root root /usr/sbin/backupd     F = file, D = directory: mode(octal) owner group path
S backup root /usr/sbin/backupd        S = service: name runs_as exec_path
C root /opt/jobs/nightly.sh            C = scheduled job: runs_as script_path
```

The attacker is the user `guest`. **Guest can write** to an entry if: guest owns it and the owner-write bit (`0200`) is set; or its group is one of guest's groups and the group-write bit (`0020`) is set; or the other-write bit (`0002`) is set.
