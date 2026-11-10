---
title: Stitch events into incidents
summary: Group scattered log events by user into activity bursts split by idle gaps and flag bursts that span several hosts.
minutes: 40
difficulty: medium
tags: security, detection, incident-response
id: sec-detection/incident-stitching
mode: stdout
---
Alerts rarely arrive as one tidy story. The same user's actions are scattered across hosts and sources, and the first job in an investigation is to stitch them into bursts of activity that an analyst can read as a unit. Your merged timeline from [building a timeline](/learn/sec-forensics/building-a-timeline) is the starting point; the new idea is the idle gap. Events from one user belong to the same burst while each is within `G` seconds of the previous one; a longer silence ends the burst and starts another.

## The method

Order matters. The input is not sorted, so sort by timestamp first, keeping the input order for events at the same second (a stable sort), then split per user. For each burst keep the first and last time, the event count and the distinct hosts in the order they were first seen. A burst that touches more than one host deserves a label, `MULTI-HOST`: movement between machines is one of the clearest signals in an intrusion, but it is also what administrators do all day, so the label is a prompt to look, not a verdict.

## Worked example

With `G = 60` and these unsorted events (`<ts> <host> <user> <message>`):

```text
1130 db2  alice query big table
1000 web1 alice login ok
1010 web1 bob   login ok
1050 app3 alice sudo used
1100 db2  alice login ok
1200 web1 alice login ok
1300 web1 bob   logout
```

```text
alice 1000-1130 n=4 hosts=web1,app3,db2 MULTI-HOST
bob 1010-1010 n=1 hosts=web1
alice 1200-1200 n=1 hosts=web1
bob 1300-1300 n=1 hosts=web1
```

After sorting, alice acts at 1000, 1050, 1100, 1130: every gap is at most 60, so they form one burst across three hosts. Her event at 1200 is 70 seconds after 1130, which is more than `G`, so it starts a new burst. Bob's two events are 290 seconds apart and become two bursts. The lines are ordered by first timestamp, then by user name, not by user.

## Pitfalls

- **Sort first.** Splitting before sorting compares each event with the wrong neighbour and invents or hides bursts.
- **Stable sort.** Events with the same timestamp keep input order; this decides which host is "first seen".
- **Exactly `G` apart.** A gap equal to `G` stays in the same burst; only a gap greater than `G` splits it. Measure from the user's previous event, not from the burst start, or long bursts get cut.
- **Per user.** One user's events never extend another user's burst.
- **Distinct hosts.** Repeats of a host count in `n` but appear once in `hosts=`, in first-seen order.
- **Single-event bursts** print `first-last` with the same number twice.

## Why defenders care

The summary is the product: an analyst reads five lines instead of five hundred. Burst boundaries and the multi-host label decide what gets a second look, so be exact about both.
