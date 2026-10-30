---
title: Capstone: chain a leak into management access
summary: Combine scoped discovery, leaked configuration and a service token to prove unauthorized management access.
minutes: 40
difficulty: hard
tags: security, offensive-lab, chained-compromise
id: sec-infra-offense/chained-compromise
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

An infrastructure compromise may require several individually modest findings. A public backup config reveals a management port and a storage key; an exposed value reveals a token; the token grants access to a privileged snapshot. The impact comes from the entire chain.

Keep scope throughout the chain. A server-provided port is untrusted evidence, not permission to contact it. The management port must still be in the caller's supplied scope. Do not turn a leaked token into a reported success until a protected operation returns fresh evidence. Remediation requires closing the leak and rotating the exposed secret; fixing only one link may leave existing credentials usable.
