---
title: "Review: one log, three questions"
summary: Answer three familiar questions about a synthetic authentication log: who guessed, who got in afterwards, and how many logins were hops.
minutes: 30
difficulty: medium
tags: review, logs, detection
id: ethical-hacking/review-intruder-log
mode: stdout
kind: review
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the authentication log on stdin is invented, with documentation addresses and made-up hosts and users. In scope: the text you read. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

This review reuses ideas you already have, on the log format from [lateral movement](/learn/ethical-hacking/lateral-movement): counting events per source (as in [the scan detector](/learn/ethical-hacking/detect-the-scan)), a threshold, a sort, and the parent rule.

Each line is `TS DST USER SRC RESULT` with `RESULT` either `OK` or `FAIL`. Time order, as before.

1. **Who guessed?** Count `FAIL` events per `SRC`. A source with **5 or more** failures is a guessing source.
2. **Who got in afterwards?** For a guessing source, the first `OK` from that same source that comes **after the fifth failure** (later in the input) is a suspected success. Report its user, host and time.
3. **How many hops?** Count the `OK` events whose `SRC` is a host somebody had logged in to (`OK` with that host as `DST`) earlier in the input within 600 seconds, as in the lateral movement lesson.

A failed login followed by a success is also what a forgetful user looks like. The log tells you what happened, never why. Say so in anything you write up.
