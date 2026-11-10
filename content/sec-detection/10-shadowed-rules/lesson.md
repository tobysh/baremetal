---
title: Find shadowed firewall rules
summary: Audit an ordered rule list for rules that can never match because an earlier rule already covers them.
minutes: 45
difficulty: hard
tags: security, detection, network, firewall
id: sec-detection/shadowed-rules
mode: stdout
---
Firewalls break quietly. An administrator adds `deny tcp 10.1.0.0/16 any 80` at the bottom, believing it blocks that network, but a rule above it already says `allow tcp 10.0.0.0/8 any any`. Every packet the new rule would match is taken by the earlier one, so the new rule is shadowed: it is dead text, and the intended protection does not exist. Shadowing is easy to introduce and invisible in normal traffic, which makes it an ideal job for a tool.

## Containment, field by field

Rule A shadows a later rule B when every packet matching B also matches A. Check each field:

- the protocol: A's is `any`, or equal to B's (a `tcp` rule does not cover a B whose protocol is `any`);
- the source network: A's network contains B's;
- the destination network: the same;
- the port range: A's range contains B's range, ends included.

Network containment with prefix lengths is `lenA <= lenB` and `(netB & maskA) == (netA & maskA)`. In mask terms, A's mask must be a subset of B's mask, and B's network must fall inside A's. Reuse the parser from [the previous lesson](/learn/sec-detection/firewall-rules), which is in your starter, so `any` is `/0` with ports `0-65535`, and non-normalised networks are masked on parsing.

## Worked example

```text
1  allow tcp 10.0.0.0/8   any 22-80
2  deny  tcp 10.1.0.0/16  any 80
3  allow tcp 10.1.0.0/16  any 80-443
4  deny  any 10.0.0.0/8   any 22
5  allow tcp 10.2.3.4/32  any 22
```

```text
rule 2 shadowed by rule 1 (conflict)
rule 5 shadowed by rule 1 (redundant)
```

Rule 3 survives because its ports reach 443, past rule 1's range. Rule 4 survives because its protocol is `any`, which is wider than rule 1's `tcp`. Rule 5 is covered by rule 1 and has the same action, so it is redundant; rule 2 has the opposite action, so it is a conflict.

## Reporting

When the shadowing rule has the same action as the shadowed one, the dead rule is merely redundant and can be deleted. When the actions differ it is a conflict: someone's intent is not being applied, and that is the finding worth a ticket. If several earlier rules cover a later one, report the lowest-numbered one.

## Pitfalls

- **Direction of containment.** The earlier rule must be the wider one. Checking the other way round reports good rules as dead.
- **Protocol `any`.** `any` covers everything, but only `any` covers `any`.
- **Equal rules.** A rule identical to an earlier one is shadowed (containment includes equality).
- **Single rule only.** This lesson reports shadowing by one earlier rule. A rule covered only by the union of several earlier rules is a harder problem we leave out, so do not flag it.
- **Port edges.** `22-80` does not contain `80-443`; check both ends.

## Why defenders care

A firewall review that only reads rules top to bottom misses dead rules. Running this audit after every change turns a silent mistake into a finding.
