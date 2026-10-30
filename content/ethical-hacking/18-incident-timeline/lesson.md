---
title: Reconstruct an incident timeline
summary: Correct per-source clock offsets, merge evidence into one timeline, label each step by attack phase and measure how long the intruder was present.
minutes: 40
difficulty: medium
tags: incident-response, timeline, forensics
id: ethical-hacking/incident-timeline
mode: stdout
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the evidence on stdin is a made-up set of log lines from invented sources (fw, web01, db03). In scope: the text you read. It describes no real intrusion. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

After an incident the first job is not blame, it is **order**: what happened, in what sequence, and for how long. Evidence arrives from many machines whose clocks disagree, so the first step in a [timeline](/learn/sec-forensics/building-a-timeline) is to bring everything onto one clock. A firewall that runs 90 seconds fast will put its log line after the event it actually caused. Wrong order means wrong conclusions about cause.

Once the lines are on a common clock you can label them with an attack **phase**. Attackers tend to move through recognisable stages, and the defender's questions follow the stages:

| Phase | What it means | Event codes |
| --- | --- | --- |
| recon | looking for what exists | `scan` |
| access | getting in | `login_fail`, `login_ok` |
| persistence | making it survive reboots (see [persistence](/learn/ethical-hacking/persistence)) | `new_startup`, `new_task` |
| lateral | moving to other hosts (see [lateral movement](/learn/ethical-hacking/lateral-movement)) | `hop` |
| exfil | taking data out | `big_upload` |

Other codes (`logout`, `noise`, anything unknown) are context: they appear in the timeline but belong to no phase. **Dwell time** is how long the intruder was inside before anyone noticed, a number every incident report quotes. Finally, if the first time seen for each phase does not follow the order above, either the attacker skipped a stage or your clocks are still wrong. Say which in your notes, never guess.

## Input

```text
# sources
fw 90            <- the fw clock is 90 s ahead of true time (use negative values for slow clocks)
web01 -12
# events
LOCALTS SOURCE message...
```
The true time of an event is `LOCALTS - offset` of its source (0 if the source is not listed).
