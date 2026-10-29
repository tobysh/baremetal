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

Order matters. The input is not sorted, so sort by timestamp first, keeping the input order for events at the same second (a stable sort), then split per user. For each burst keep the first and last time, the event count and the distinct hosts in the order they were first seen. A burst that touches more than one host deserves a label, `MULTI-HOST`: movement between machines is one of the clearest signals in an intrusion, but it is also what administrators do all day, so the label is a prompt to look, not a verdict.

The summary is the product: an analyst reads five lines instead of five hundred.
