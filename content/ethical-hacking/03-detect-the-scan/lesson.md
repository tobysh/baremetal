---
title: Detect the scan in the firewall log
summary: Read a synthetic firewall log and flag sources that touch many distinct ports in a short window, without flagging noisy but harmless retries.
minutes: 25
difficulty: medium
tags: detection, logs, scanning
id: ethical-hacking/detect-the-scan
mode: stdout
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the firewall log on stdin is canned and uses documentation addresses (192.0.2.x, 198.51.100.x, 203.0.113.x). In scope: the text you read. You touch no network. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

Your [port scanner](/learn/ethical-hacking/port-scanner) leaves a distinctive footprint: one source address touching **many different ports** in a short time. A defender who knows this can spot it in a firewall log.

Each log line is `TIMESTAMP SOURCE ACTION PROTO PORT`, for example `1700000012 203.0.113.5 DENY tcp 22`. Timestamps are Unix seconds and never decrease. `ACTION` is `ALLOW` or `DENY`.

## The rule

A **scan** is a source that, within any **10-second window**, contacts at least **8 distinct destination ports**, whatever the action. The window at an event with time `t` holds that source's events with `t - 10 < ts <= t`, up to and including the event being read.

Two traps make this more than a counter:

- A client retrying the same port a hundred times is noisy but is **not** scanning. Count *distinct* ports, not events.
- A scanner that sends one probe every few seconds slips under a short window. That is a real **detection gap**: it is why defenders also run a longer window. Your rule is a starting point, not a guarantee.

Report each scanning source once, at the first moment it crosses the threshold. Looking at the `ALLOW` events in the same window also tells you whether the scan **found** something: a distinct port with an `ALLOW` is an exposure to close.
