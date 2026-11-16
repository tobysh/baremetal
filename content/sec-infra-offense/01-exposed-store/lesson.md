---
title: Discover an exposed data store
summary: Enumerate scoped toy services and prove unauthorized access by reading a fresh synthetic marker.
minutes: 40
difficulty: hard
tags: security, offensive-lab, exposed-store
id: sec-infra-offense/exposed-store
mode: harness
---

> **Simulated target.** Everything in this lesson is made up and runs inside the sandbox: the only 'host' is a `Lab` object the tests pass to your code, and `Lab.Send` is a plain Go callback, not a socket. In scope: the ports the exercise hands you. Nothing here touches a real service. Never use a technique from this course on a system you do not own or do not have written permission to test. Start with [Law, ethics and permission](/learn/ethical-hacking/law-and-ethics) if you have not yet.

This is the first lesson of the track, so we go slowly. The idea is small: **a banner is a clue, a successful read of protected data is a finding.** Your code has to tell the two apart.

## The model

You never open a connection. You build a `Request` (port, method, path) and hand it to `lab.Send`, which returns a `Reply` (status, body, items). Statuses look like HTTP so they are easy to read, but nothing speaks HTTP.

```text
you                          lab (one toy service per port)
 |-- GET /banner  --------->  200 "labkv/1.0"      "I am a key-value store"
 |-- GET /keys    --------->  200 items: [public, backup-7]
 |-- GET /value/backup-7 -->  200 "LAB-PROOF:..."   data came back, no token needed
```

A hardened store with the same banner answers `/keys` with `401` instead. The banner is identical, so the banner alone proves nothing.

## A worked example

Scope is `[6012, 6011]`. A careless scanner would stop at the first banner it likes. Walk it properly:

```text
port 6012: GET /banner  -> 200 "labhttp/1.0"   not a store, skip, no more requests here
port 6011: GET /banner  -> 200 "labkv/1.0"     a store, continue
port 6011: GET /keys    -> 200 [public, backup-3]
port 6011: GET /value/public   -> 200 "public documentation"   no LAB-PROOF: prefix, ignore
port 6011: GET /value/backup-3 -> 200 "LAB-PROOF:store-3"      proof: return Finding{6011, that body}
```

The skeleton in Go:

```go
for _, port := range ports {
    b := lab.Send(Request{Port: port, Method: "GET", Path: "/banner"})
    // continue unless 200 and exactly "labkv/1.0"
    // then list keys, then read each value in order
}
```

## Pitfalls

- Sending `/keys` to a port whose banner was not `labkv/1.0`. The tests treat that as touching a decoy.
- Returning on the first key without looking at the body. Only a `LAB-PROOF:` prefix counts.
- Giving up when one store is hardened. Later ports may still be vulnerable; stop only when the list is exhausted.
- Hardcoding a marker. Ports, keys and markers differ in every scenario.

## The defender's half

The same transcript, read from the other side, is an audit finding: an unauthenticated data service reachable from where it should not be. The fixes are layered. Require authentication and authorization on every data operation (`/keys` as well as `/value/...`), bind the service to the interfaces that need it, and firewall the port. Hiding the banner is not on that list; it changes the clue, not the exposure.
