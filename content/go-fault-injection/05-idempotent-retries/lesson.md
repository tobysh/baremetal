---
title: Retries Without Duplicate Effects
summary: A timeout does not tell you whether the work happened. Make the server idempotent by request id and the client retry with a bounded budget and a stable id.
minutes: 35
difficulty: hard
tags: retries, idempotency, fault-injection
id: go-fault-injection/idempotent-retries
mode: harness
---
When a request times out you know only that no reply arrived. The server may never have received it, or it may have done the work and the reply was lost. The client cannot tell the two apart, so retrying can **apply the effect twice**: a customer is charged double.

## Two halves of the fix

**On the server**, make the operation idempotent with a client-chosen **idempotency key**. The first request with a key does the work and remembers the key. A repeat with the same key and the same parameters does nothing and reports success. The same key with different parameters is a bug in the client and gets `ErrConflict`.

**On the client**, retry only errors that mean "outcome unknown" (`ErrTimeout`), send the **same key** on every attempt, and stop after a fixed budget. Permanent errors such as `ErrInsufficient` are returned at once: retrying cannot change them.

## The three failure points

The hidden transport can do one of three things on each attempt:

| Outcome | Server state | Client sees |
| --- | --- | --- |
| ok | applied | success |
| lost request | unchanged | `ErrTimeout` |
| lost reply | applied | `ErrTimeout` |

The test tries **every schedule** of these for up to four attempts and every attempt budget from 0 to 4, and compares with an expected result worked out by hand-rolled rules: the balance changes by exactly the amount or not at all, the number of calls never exceeds the budget, and success is reported only when a reply actually arrived.

## What this does not prove

Idempotency keys must be remembered for as long as clients may retry. This lesson keeps them in a map forever; a real service must expire them and decide what a retry after expiry means. A lost-reply timeout can also end with `ErrGaveUp` even though the work was done: the honest result is "outcome unknown", which is why the error says so.
