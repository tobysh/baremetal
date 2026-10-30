---
title: Test an authorization matrix
summary: Replay scoped operations as another account and distinguish confirmed access from misleading success codes.
minutes: 35
difficulty: medium
tags: offsec, web, assessment
id: ethical-hacking/authorization-matrix
mode: harness
---
> **Simulated target.** Only the in-memory callback supplied by this lesson is in scope. All accounts, files and markers are fictional. No network requests or host file access are needed. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics).

An assessment begins with a hypothesis: can a second ordinary account perform an operation reserved for the owner? Replaying the same operation with another identity changes one variable and gives stronger evidence than guessing object numbers.

In this lab you receive a list of approved probes. Each has an owner identity, a second identity and a fictional object. The callback returns a status and an `effect` marker. For reads, the marker means the protected record was returned; for writes, it means the synthetic change happened. A `200` containing a login page has no effect marker and proves nothing.

Run a positive control as the owner first. If it fails, record an inconclusive result and do not run the second request. Otherwise replay as the other identity. Confirm a bypass only when the replay returns a 2xx status **and** the protected effect. A denial is a pass for this check, not evidence that every route is secure.

For example, an owner receives `{200, 1}` when reading fictional object 47. A second account receives `{200, 0}`: no bypass is confirmed, because no protected data was returned. If that account receives `{200, 1}`, record a bypass. If the owner receives `{404, 0}`, the control is inconclusive; do not interpret the second account's access.

Keep the probes in their supplied order. The harness resets each simulated request so a write cannot contaminate the next control. Real assessments need equivalent disposable fixtures and explicit permission for state-changing tests.

After a fix, replay the same pair: the owner should still succeed and the other account should be denied. The remediation is server-side authorization on every operation using the authenticated identity, including alternate write routes.

Further reading: [OWASP authorization testing](https://wstg.owasp.org/stable/4-Web_Application_Security_Testing/05-Authorization_Testing/).
