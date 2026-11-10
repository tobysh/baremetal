---
title: Correlate authentication failures
summary: Summarize password failures by source and flag the first later password acceptance after a threshold.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/auth-log-analysis
mode: stdout
---
## The forensic question

The authentication log of a server shows hundreds of lines. Is someone guessing passwords, from where, and did the guessing end in a login? Answering means parsing the event grammar, not counting the word "Failed".

## The data

Synthetic `sshd` lines look like this (the host is fictional and the addresses come from the documentation ranges):

```text
Mar  3 02:01:27 example-host sshd[9280]: Failed password for invalid user admin from 198.51.100.7 port 24364 ssh2
Mar  3 02:02:27 example-host sshd[2251]: Failed password for deploy from 198.51.100.7 port 1965 ssh2
Mar  3 02:04:10 example-host sshd[4741]: Accepted publickey for alice from 192.0.2.10 port 58891 ssh2
Mar  3 02:04:52 example-host sshd[5865]: Accepted password for bob from 192.0.2.11 port 16628 ssh2
Mar  3 03:00:01 example-host CRON[3488]: pam_unix(cron:session): session opened for user root
```

Only two patterns count: `Failed password for [invalid user ]USER from IP ...` and `Accepted password for USER from IP ...`, both from `sshd`. Public-key events, connection notices and other programs are noise.

## Worked example

Suppose address `198.51.100.7` produces these password failures in order: `admin`, `test`, `deploy`, `guest`, `root`, then `Accepted password for alice from 198.51.100.7`.

1. Failures one to four: the address is below the threshold of five.
2. Failure five (`root`): the address becomes a **candidate**: `failures=5 users=5`.
3. The acceptance of `alice` arrives after that point, so the candidate is annotated `ACCEPTED as alice`.

The output row is `198.51.100.7 failures=5 users=5 ACCEPTED as alice`. Had the acceptance come after the fourth failure, it would not count: the address had not yet reached the threshold. Later failures and later acceptances do not change which acceptance is reported, only the first one after the threshold counts.

Rows are sorted by failure count descending, ties broken by `strcmp` on the address text, so `198.51.100.140` sorts before `198.51.100.23`. If nothing qualifies print `no brute force detected`.

## Pitfalls

- **`invalid user`.** Skip that literal phrase when extracting the user. A user who is literally named `invalid` (`Failed password for invalid from ...`) is a normal user.
- **Match the program.** A `su` or `login` line may contain the same words; only `sshd[` lines count.
- **Order matters.** Record an acceptance while scanning, so an earlier login cannot be attached after later failures.
- **Distinct users.** Keep a per-address set of usernames; repeating `admin` ten times is `users=1`.
- **Text order.** The tie break is lexicographic, not numeric IP order.

## What a defender concludes

An address with many failures across many usernames followed by a success is worth an urgent look: reset that account, review what the session did and check for the same source elsewhere. But the log shows only correlation. NAT may hide many people behind one address, and an attacker may log in from a different one. State the observation (address, counts, first accepted user) and keep the cause as a hypothesis.
