---
title: Test weak management credentials
summary: Use a bounded lab dictionary and verify the returned token grants a protected management read.
minutes: 40
difficulty: hard
tags: security, offensive-lab, management-login
id: sec-infra-offense/management-login
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

A management login may be exposed because operators kept weak or default credentials. In a scoped assessment, a small explicitly permitted dictionary can test this hypothesis. This lab supplies both the username and the complete permitted password list; it does not search real accounts.

A login response is not enough. A service might return a success-looking status with no usable session, or authenticate a role that cannot access administration. Prove the returned token can read the protected synthetic marker. Remediation includes unique credentials, restricted management exposure and authorization at the protected operation.
