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


## The model

```text
POST /accounts {name, password, UID 0, Hidden true}   -> 201
GET  /accounts/all      -> [... name ...]      present: the account exists
GET  /accounts/visible  -> [...]               absent: the normal view hides it
POST /login {name, password} (no token)        -> 200 + new session token
GET  /root/proof  (new token)                  -> 200 "LAB-PROOF:..."
```

The two listings answer different questions. `visible` is what an everyday administrator sees; `all` is the audit view. An account that appears only in `all` is the persistence finding, and the login with the *new* credentials proves it works without the original admin token.

## A worked example

Account `lab-ghost`, password `p-17`:

```text
POST   /accounts            admin token, JSON account        -> 201
GET    /accounts/all        admin token                      -> ["admin","lab-ghost"]
GET    /accounts/visible    admin token                      -> ["admin"]
POST   /login               {"user":"lab-ghost","password":"p-17"}  -> 200, Token "s-9"
GET    /root/proof          Token s-9                        -> 200 "LAB-PROOF:acct-1"
DELETE /accounts/lab-ghost  admin token
GET    /accounts/all        -> ["admin"]                     gone
POST   /login               same credentials                 -> 401
GET    /root/proof          Token s-9                        -> 401 or 403   session revoked
```

The last three lines matter as much as the first six. Deleting an account does not by itself end a session that is already open; the model revokes sessions on deletion, and you must prove it did.

## Pitfalls

- Accepting the `201` as evidence. Creation proves a write, not access.
- Checking only `all` or only `visible`. You need present in one and absent from the other.
- Using the admin token for the proof read. That shows the admin can read the marker, not that the planted account works.
- Skipping cleanup when verification fails. Once creation succeeded, delete the account regardless.
- Forgetting the old session. Deleting the account and testing only a fresh login leaves open whether the issued token still works.

## The defender's half

Compare the account store with what the management UI shows, and alert on any UID 0 account other than the expected one. Review accounts created outside change windows, and make deletion revoke sessions and keys. Real hiding techniques differ, and this toy flag is not a claim about any real system. The audit habit of comparing two views of the same data is what transfers.
