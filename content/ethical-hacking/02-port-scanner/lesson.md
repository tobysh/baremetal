---
title: A port scanner for a simulated network
summary: Write a scanner against an in-process fake host, handle timeouts and retries, then audit its result against what the host is supposed to expose.
minutes: 25
difficulty: easy
tags: recon, scanning, audit
id: ethical-hacking/port-scanner
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the only 'host' is a function the tests pass to your code. In scope: that function. Nothing here opens a socket. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

**Reconnaissance** is the first thing an attacker does and the first thing a defender audits: what is listening? A **port scan** asks, for each port number, "is anything answering?". A real scanner sends packets. Yours calls a **probe function**, which the tests replace with a model of a host, so you learn the logic without any traffic.

A probe has three answers:

- `PORT_OPEN` (1): a service accepted the connection.
- `PORT_CLOSED` (0): the host actively refused.
- `PORT_TIMEOUT` (-1): silence. Either a firewall dropped the probe (the port is **filtered**) or a packet was lost.

Silence is ambiguous, so a careful scanner **retries**. A single lost packet should not hide an open port. But it should not retry forever either: a scan that hammers one port is slow and noisy. Allow at most **two retries** (three attempts) and then call the port filtered.

## The defender's half

A list of open ports is only useful next to a list of ports that *should* be open. Anything extra is an **unexpected exposure**: an old test service, a debug port, a database bound to every interface. `unexpected_open` is the audit, and it is the first thing you do on your own systems. The next lessons show how a scan looks from the other side, in [detecting the scan](/learn/ethical-hacking/detect-the-scan) and [rate limiting](/learn/ethical-hacking/rate-limiting).
