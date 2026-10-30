---
title: Probe mass assignment
summary: Change one account field at a time and use a fresh read to prove whether protected state changed.
minutes: 35
difficulty: medium
tags: offsec, web, assessment
id: ethical-hacking/mass-assignment
mode: harness
---
> **Simulated target.** Only the in-memory callback supplied by this lesson is in scope. All accounts, files and markers are fictional. No network requests or host file access are needed. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics).

A profile update may copy every submitted field into an account record. When that includes fields such as `role` or `credit`, an ordinary edit becomes an authorization failure called mass assignment.

In this lab you assess an in-memory account service through update and read callbacks. The owner supplies a list of approved field/value probes. The fixture restores its baseline before each update, so every probe is independent. A fresh read after the update supplies the evidence: acceptance is not the same as persistence.

Check a permitted `display_name` update first. Then submit each candidate and compare protected fields against the known baseline. A 403 response can still hide a bug if the service mutates state before rejecting the request. Conversely, a 200 response that ignores an unknown field is not proof of mass assignment. Inspect state after every update regardless of its response code.

Return evidence for the first protected-state change and stop. Report which field caused the change, the baseline, and the observed value; use synthetic data in the report. In a real application, the proof should use a disposable account and avoid granting lasting privileges or affecting other users.

The fix is an explicit server-side allowlist of editable fields plus authorization for privileged transitions. Retest that display-name edits work while protected fields remain unchanged. These probes test the supplied fields only; they are not a complete review of nested objects or every update endpoint.

Further reading: [OWASP authorization testing](https://wstg.owasp.org/stable/4-Web_Application_Security_Testing/05-Authorization_Testing/).
