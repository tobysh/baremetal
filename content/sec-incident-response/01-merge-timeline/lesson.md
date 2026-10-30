---
title: Merge logs into one timeline
summary: Normalize time zones, correct known clock skew and merge sources into a deterministic UTC timeline, rejecting malformed lines.
minutes: 30
difficulty: medium
tags: incident-response, timeline, logs
id: sec-incident-response/merge-timeline
mode: harness
---

Everything in this track is invented: the company, hosts, users and addresses. The code only reads data handed to it by tests.

Responders rarely get one log. A web server, a login service and a file server each write their own timestamps, in their own time zone, from their own clock. The first job is to put every record on one clock, because an order you cannot defend makes every later conclusion shaky.

Two different corrections are involved. A **time zone offset** is part of the timestamp (`2026-03-04T11:15:00+01:00` is `10:15:00Z`), so parsing handles it exactly. **Clock skew** is different: a machine's clock runs ahead of or behind true time, and the only way to know by how much is an outside reference, for example a record of the same event on a trusted clock. Here each source comes with a skew estimate in seconds. That estimate is an assumption you carry into the timeline, not an observation, so an analyst writes it down next to the result.

Two records can land on the same corrected second. When they do, pick a rule and keep it, otherwise two runs on the same data may disagree. Records that cannot be parsed are not silently dropped: they are listed, because a gap in the evidence is itself a finding.

**Limitation:** a skew estimate has its own error. Two events a few seconds apart in different sources cannot be ordered with confidence even when the sort places one first.
