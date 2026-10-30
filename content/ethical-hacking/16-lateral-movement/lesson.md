---
title: Lateral movement in the authentication log
summary: Link successful logins into hop-by-hop chains across hosts in a synthetic log, so a single stolen credential walking through the network becomes visible.
minutes: 40
difficulty: hard
tags: lateral-movement, detection, logs, incident-response
id: ethical-hacking/lateral-movement
mode: stdout
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the log on stdin is invented, with made-up host names (ws-17, web01, app02, db03) and users. In scope: the text you read. No machine is logged into and no credential is used. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

An attacker rarely lands on the machine they actually want. A phished laptop is not the database. So after the first foothold they **move laterally**: use a credential or a trust relationship to log in to the next host, then the next, hunting for something more valuable. Each hop is an ordinary-looking successful login. Alone, none is suspicious. Together they form a path.

The defender's insight is to read the **authentication logs of many hosts as one graph**. A login to `app02` whose *source* is `web01`, a few minutes after someone logged in to `web01`, ties the two events together. A normal user rarely hops through two or three servers in ten minutes; an intruder following a trail does it all the time.

## Input and rule

One event per line, in time order: `TS DST USER SRC RESULT`, for example `1700000100 web01 alice ws-17 OK`. `TS` is Unix seconds, `DST` the host logged in to, `SRC` the host the login came from, and `RESULT` is `OK` or `FAIL`. Ignore failures and malformed lines (only successes can be hops).

The **parent** of a success event `e` is the *most recent earlier* success `p` (earlier in the input) where `p.DST == e.SRC` and `e.TS - p.TS <= 600`. In words: this login came from a host that somebody had just logged in to. An event without a parent starts a chain. A **chain** is reported at its last event (the *leaf*, an event that is nobody's parent) when it has at least two hops.

Report in the order of the leaves in the input. Mitigations belong in your notes, and will matter later: separate admin credentials per tier, no shared local passwords, network segmentation so `web01` cannot log in to `db03` at all, and multi-factor authentication for remote logins. Then compare this with [the timeline](/learn/ethical-hacking/incident-timeline) you will build from the same kind of data.
