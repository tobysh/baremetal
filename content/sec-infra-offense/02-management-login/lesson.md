---
title: Test weak management credentials
summary: Use a bounded lab dictionary and verify the returned token grants a protected management read.
minutes: 40
difficulty: hard
tags: security, offensive-lab, management-login
id: sec-infra-offense/management-login
mode: harness
---

> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the only 'host' is a `Lab` object the tests pass to your code, and `Lab.Send` is a plain Go callback, not a socket. In scope: the one port and the short password list the exercise hands you. No real login, account or credential is involved. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

Management interfaces are often left with default or weak passwords. In a real assessment the client agrees in writing to a **small, named list** of candidates, and the tester stays inside it. This lesson models that: you get a user name and at most 16 permitted passwords, nothing else.

## The model

A login is a `POST /login` with a JSON body. A reply can mean three different things, and only the last is a finding:

```text
POST /login {"user":"operator3","password":"wrong"}    -> 401              not it
POST /login {"user":"operator3","password":"decoy"}    -> 200 token=reader  a session, but of what?
POST /login {"user":"operator3","password":"lab-key"}  -> 200 token=admin-s a session
GET  /admin/proof  (token=reader)                       -> 403              low role, not a finding
GET  /admin/proof  (token=admin-s)                      -> 200 "LAB-PROOF:..." protected read
```

A session token is only a candidate until you use it on something that requires the role you care about.

## A worked example

Passwords are `["wrong", "decoy", "lab-key"]`.

```text
try wrong   -> 401                    next
try decoy   -> 200 + token "reader"   candidate: GET /admin/proof -> 403, next
try lab-key -> 200 + token "s-3"      candidate: GET /admin/proof -> 200 LAB-PROOF:admin-3, done
```

In Go, build the body with `encoding/json` so quotes or backslashes in a password cannot break the JSON:

```go
body, _ := json.Marshal(map[string]string{"user": user, "password": pw})
r := lab.Send(Request{Port: port, Method: "POST", Path: "/login", Body: string(body)})
```

## Pitfalls

- Treating `200` as success. A "reader" session is a login, not a management compromise.
- Trying a password twice, or one that is not in the list. Scope is part of the result.
- Continuing to try passwords after the protected read has already succeeded. One proof is enough.
- Putting the password in the evidence. A report quotes the marker and the endpoint, never the secret.
- Hand-building JSON with string concatenation.

## The defender's half

Weak management credentials are found by checking, not by guessing: unique strong credentials per device, no vendor defaults, management ports reachable only from an admin network, and rate limits plus lockout so a short list cannot grow into thousands. The second lesson in this transcript matters too: authorization must be checked at the protected operation, so that a valid but low-privilege session still gets 403.
