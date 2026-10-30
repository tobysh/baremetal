---
title: Establish and remove modeled account persistence
summary: Create an account omitted from the normal listing, prove fresh-login access and remove it through the full audit view.
minutes: 40
difficulty: hard
tags: security, offensive-lab, account-persistence
id: sec-infra-offense/account-persistence
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

Persistence means retaining a way back after the original access path disappears. A new privileged account can provide that access. A normal user-listing interface may omit certain accounts; that omission is not the same as the account being absent from the account database.

Here `Hidden` is an explicit flag in the fictional management application. It hides the account from `/accounts/visible` but not `/accounts/all`. This does not claim that a flag or name universally hides a real Linux account. The toy UID 0 account gets the model's administrative role. No account is created in the runner.

A successful creation reply is weak evidence. Verify the account in the full audit view, its absence from the normal view, and a new authenticated session that can read the root marker. Then delete it, verify it is absent, and verify both its credentials and issued session no longer work. Hidden accounts are still audit findings, and responsible tests include this cleanup.
