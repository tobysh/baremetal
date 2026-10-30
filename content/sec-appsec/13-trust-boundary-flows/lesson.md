---
title: Check a data-flow model for trust-boundary problems
summary: Walk a list of components and data flows and report missing encryption, missing authentication, exposed secrets and flows that skip the DMZ.
minutes: 35
difficulty: easy
tags: security, threat-modelling
id: sec-appsec/trust-boundary-flows
mode: stdout
---
Threat modelling asks, before the code exists: what are we building, what can go wrong, what will we do about it. A data-flow diagram is the usual starting point: components, the flows between them and the trust boundaries where the level of trust changes. Most real problems sit on those boundaries, and a checklist run over the diagram finds the obvious ones cheaply. The STRIDE categories (spoofing, tampering, repudiation, information disclosure, denial of service, elevation of privilege) give the vocabulary; here we apply four concrete rules that map onto them.

Components belong to zones with a trust level: `internet` 0, `dmz` 1, `internal` 2, `admin` 3. A flow carries data of a classification (`public`, `internal` or `secret`), is or is not encrypted in transit and is or is not authenticated. Rules, in this order of reporting: `no-encryption` when the flow is not encrypted and either crosses zones or carries non-public data (tampering and disclosure); `no-authentication` when it is not authenticated and moves towards a more trusted zone (spoofing and privilege); `secret-exposed` when secret data goes to a component in a zone below `internal`; and `skips-dmz` when a flow goes directly from the internet to an internal or admin component. Fix the design, not the diagram: each finding is a decision to record and review.

Nothing here replaces thinking about how an attack would proceed; the [attack tree lesson](/learn/sec-appsec/attack-tree-cost) adds cost reasoning to that. It does, however, give your design reviews a repeatable minimum.
