---
title: Cost an attack tree
summary: Compute the cheapest attack through an AND/OR tree before and after mitigations, and list the steps on the cheapest path.
minutes: 50
difficulty: hard
tags: security, threat-modelling, analysis
id: sec-appsec/attack-tree-cost
mode: stdout
---
An attack tree breaks one goal ("read the customer database") into the ways an attacker could reach it. An OR node is satisfied by any child; an AND node needs all children. Each leaf is a concrete step with a cost: effort, money or risk, in whatever unit the team agrees. Annotated with costs, the tree answers a useful question: what is the cheapest way in, and does a proposed defence change it?

Cost propagates up the tree. A leaf costs its own cost. An AND node costs the sum of its children (the attacker needs all of them). An OR node costs the minimum child (the attacker picks the cheapest). A mitigation makes a leaf impossible, modelled as infinite cost: an AND with an impossible child is impossible, while an OR is still possible if some other child is. This is why one fix rarely closes a goal; it just moves the cheapest path, and the interesting result is the new cheapest path and how much dearer it is.

Compute the cost twice, once ignoring mitigations and once honouring them, and report the leaves of the cheapest remaining path. If two OR children cost the same, take the one listed first. The units are judgement calls and the tree is only as complete as its authors' imagination, but making the argument explicit is exactly what a good [threat model](/learn/sec-appsec/trust-boundary-flows) review wants. It is also the quantitative cousin of the [rule shadowing](/learn/sec-detection/shadowed-rules) audit: both look for the path you forgot was open.
