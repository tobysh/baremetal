---
title: Claims, evidence and assumptions
summary: Grade analyst claims against an evidence set as supported, reported, contradicted, inferred or unsupported, with explicit assumptions.
minutes: 35
difficulty: medium
tags: incident-response, evidence, reasoning
id: sec-incident-response/claims-and-evidence
mode: harness
---

An incident report mixes four different things: what a system recorded (observation), what a person said (a report), what you concluded by reasoning (inference) and what you simply do not know. Running them together is how a plausible story hardens into a wrong finding.

Treat each claim as a statement about a subject and an attribute, for example "host `web1` had `listener` = `4444`". Evidence either records that exactly (and was observed by a system or tool) or it does not. A user's email saying so is only a report. A record about something nearby only supports the claim if you add a step, and that step must be written down as an assumption so a reviewer can challenge it.

This lesson is a **structure check**. It can tell you whether a claim cites evidence, whether the evidence matches, and whether leaps are declared. It cannot judge whether an assumption is reasonable. That judgement stays with a human reviewer.

**Simplification:** in this model every attribute has one true value for the whole case, so any observation with a different value contradicts the claim. Real state changes over time, and the timeline lessons handle that.
