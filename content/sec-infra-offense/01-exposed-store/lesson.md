---
title: Discover an exposed data store
summary: Enumerate scoped toy services and prove unauthorized access by reading a fresh synthetic marker.
minutes: 40
difficulty: hard
tags: security, offensive-lab, exposed-store
id: sec-infra-offense/exposed-store
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

Reconnaissance starts with scope, then evidence. A familiar port number or service banner is a clue, not a finding. This toy lab has several ports, an unauthenticated key-value service and decoys. You must enumerate only the supplied ports and prove a read of a value that should require authentication.

The vulnerable store exposes its key list and values to any caller. A hardened store may advertise the same banner but deny those reads. The test marker changes in each scenario, so a hardcoded result cannot demonstrate access. The fix is authentication and authorization for every data operation, plus appropriate service exposure controls.
