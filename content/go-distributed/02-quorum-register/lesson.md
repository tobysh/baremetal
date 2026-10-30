---
title: Quorum Replication
summary: Keep a register on several replicas, write and read through majorities, and stay correct while replicas are down or their acknowledgements are lost.
minutes: 40
difficulty: medium
tags: distributed, replication, quorum
id: go-distributed/quorum-register
mode: harness
---
One copy of data is one crash away from being lost. Keeping `N` copies on separate machines fixes that, and creates a new problem: the copies can disagree, and some may be unreachable.

## Majorities overlap

Pick a **quorum** of `N/2 + 1` replicas. Any two majorities of the same set share at least one member. If every write is stored on a quorum and every read asks a quorum, then every read talks to at least one replica that saw the latest completed write. That replica's answer is the newest value, *if you can recognise it*, so each stored value carries a **version number** and a replica only keeps the highest version it has seen.

```
replicas:   A       B       C
write v5   v5      v5      v4      <- C was down; the write still succeeded (2 of 3)
read       v5 (A)  v4 (C)          <- any 2 replicas include A or B
```

## Three traps

1. **A failed write is not a no-op.** If the write reached one replica and the acknowledgements were lost, the value is stored but the caller saw an error. Readers may see it. Your code must therefore never reuse its version number: the next write must use a *higher* version than any attempted so far, otherwise two different values could end up sharing version 5 on different replicas.
2. **Reads must repair.** A read that returns the value of a partially written version, and then a later read that returns the older one, makes the register appear to go back in time. Before returning, **write the winning value back** to the replicas and require a quorum of acknowledgements. After that, every later read must see it.
3. **Success needs evidence.** A write returns nil only when a quorum of `Put` calls *returned success*. A `Put` that returned an error might still have been applied, but you cannot count it.

## The API

A `Peer` is a replica as seen over a network: `Get` and `Put` return an error when the replica cannot be reached or the reply is lost. `Replica` is supplied and keeps the highest-versioned value it has been given.

## Limits of this model

This register has one writer (one `Register` value). Several concurrent writers need more machinery to choose versions. It also tolerates unreachable replicas, not replicas that lie.
