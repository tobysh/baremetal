---
title: Retries and Idempotency
summary: A client that retries over a lossy network and a server that applies each request exactly once, whatever the network duplicates or reorders.
minutes: 35
difficulty: medium
tags: distributed, retries, idempotency
id: go-distributed/retries-idempotency
mode: harness
---
A network can lose a request, lose the reply, deliver a message twice, or deliver an old message late. A client that sent a request and heard nothing cannot tell *which* of these happened. The server may or may not have acted.

The only safe client behaviour is to **retry until it hears an answer**. That is only safe if the server treats a repeated request as the *same* request. An operation with that property is **idempotent**: applying it twice has the same effect as applying it once.

"Add 10 to the balance" is not idempotent. You make it idempotent by giving each request an identity, `(Client, Seq)`, and having the server remember what it already did.

## The protocol in this lesson

- Each client sends numbered requests `1, 2, 3, ...` and sends request `n+1` only after the reply to request `n` arrived. So the server sees, per client, a non-decreasing sequence with at most one new request at a time.
- A reply carries the same `Client` and `Seq` as the request, plus the account balance right after the request was applied.

## Two halves

**Server.** For each client, remember the last sequence number applied and the reply that was produced. A request with the next sequence number is applied. A request for the latest sequence number again is a duplicate: do not apply it, return the *same reply as before*. An older duplicate is already acknowledged and ignored by the client; do not apply it either.

**Client.** `Pending()` returns the request to (re)send right now, and it returns the *same* request every time until a matching reply arrives. A reply is only useful if its `Client` and `Seq` match the request in flight. A late reply for request 3 must not mark request 4 as done.

## What the hidden harness does

A seeded scheduler runs several clients against one server. For a few hundred steps it drops, duplicates and reorders requests and replies, then it lets the network settle. After every delivery it checks that the balance is exactly the sum of the requests applied so far, once each.

## Limits of this model

The server keeps its table in memory, so a server restart would forget it. Lesson 6 deals with what must be written to disk. Here one client has one request in flight; real systems allow a window and need to remember more.
