---
title: Triage and severity scoring
summary: Turn a pile of findings into an ordered work queue with a transparent scoring formula, severity bands and a stable sort.
minutes: 35
difficulty: medium
tags: triage, risk, prioritisation, reporting
id: ethical-hacking/triage-severity
mode: harness
---
> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the findings are made-up records created by the tests, and the formula is a teaching formula, not an industry standard. In scope: those records. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

A real assessment produces dozens of findings and a team with limited time. **Triage** decides what to fix first. The same flaw can be urgent on one system and a footnote on another, so severity depends on *context*, and a good score is **transparent**: a reader can see why a finding ranks where it does, and argue with the inputs rather than with a magic number.

This lesson uses a small teaching formula (real standards such as CVSS are more detailed, and you should learn one in practice). Each finding has:

| Field | Range | Meaning |
| --- | --- | --- |
| `impact` | 0 to 3 | what an attacker gains: none, low, medium, high |
| `exploitability` | 0 to 2 | 0 needs local access, 1 needs a valid login, 2 works unauthenticated over the network |
| `exposure` | 0 to 2 | 0 internal only, 1 partners, 2 reachable from the internet |
| `asset_value` | 1 to 3 | how much the affected system matters |
| `exploited` | 0 or 1 | 1 if attacks on it are known to happen |
| `mitigated` | 0 or 1 | 1 if a compensating control is already in place |

The score is computed in this order, with integer arithmetic:

1. `base = impact*4 + exploitability*3 + exposure*2`
2. `score = base * asset_value`
3. `+10` if `exploited` (known attacks matter more than theory)
4. halve it (integer division by 2) if `mitigated`
5. if `impact == 0` the score is `0` whatever else is set.

Values outside the ranges are **clamped** into them (an input mistake should not produce a nonsense score).

Bands: `critical` at 40 or more, `high` from 25, `medium` from 12, `low` from 1, otherwise `info`.

The queue sorts by score (highest first), then by exposure (highest first), then by `id` (`strcmp` order), so the result is the same every time. A deterministic order is itself a feature: two people triaging the same list must end up with the same queue. The next lesson turns the top of the queue into [written reports](/learn/ethical-hacking/finding-report).
