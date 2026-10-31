---
title: "Capstone: Test an Engine Under a Failure Schedule"
summary: Write a checker that injects short writes, lost writes and crashes at every append of a small log-structured store and reports every broken invariant.
minutes: 60
difficulty: hard
tags: capstone, fault-injection, storage-engine, testing
id: go-fault-injection/capstone-fault-schedule
mode: harness
---
You now know the failures. This lesson makes you build the tool: a deterministic checker that takes **any** storage engine and a list of writes, explores a failure schedule, and returns a list of violated invariants. Hidden tests run your checker against one correct engine family and five broken ones. A good checker finds nothing wrong with the correct engines and something wrong with every broken one.

## The engine under test

The starter contains `LogStore`, a small append-only key-value store on a `FaultDisk`. Each `Put` appends one record `len | crc | klen | key | value` and updates an in-memory map. `OpenLog(d)` rebuilds the map by replaying the log, stops at the first incomplete or checksum-failing record, and **truncates the damaged tail** so later appends stay reachable. Read `LogStore` before you test it.

## The fault disk

`FaultDisk.Append` counts calls from 0. A `Fault{At, Kind, Keep}` hits exactly one call:

| Kind | Effect |
| --- | --- |
| `Short` | writes part of the data, returns the count and `ErrIO`; the disk keeps working |
| `Lost` | writes nothing, returns `ErrIO`; the disk keeps working |
| `Crash` | writes part of the data (maybe all of it) and the disk is dead (`ErrCrash`) until `Restart` |

`Keep` says how much of the data a `Short` or `Crash` writes: `KeepZero`, `KeepOne`, `KeepHalf`, `KeepAllButOne` or `KeepAll` (only meaningful for `Crash`: the data is durable but the reply is lost).

## Invariants your checker must test

Run the puts in order and stop at the first error. Remember which puts returned `nil` (**acknowledged**) and which one failed (**in flight**). Restart the disk, reopen the engine, and check:

1. **Acknowledged writes survive.** For every key, the value is the last acknowledged one, except that if the in-flight put is for the same key the new value is also allowed.
2. **Nothing is invented.** A key with no acknowledged put may only be present if it is the in-flight key, and then only with the in-flight value. A truncated or mixed value is a violation.
3. **The engine is usable after recovery.** `Put("post-recovery", "ok")` succeeds.
4. **Recovery is stable.** After restarting and reopening again, every key shows the same value as before the second restart and `post-recovery` is present.
5. **No panics.** A panic in `open`, `Put` or `Get` is a violation, not a crash of the checker.

Enumerate the schedule yourself: first run the puts on a fault-free disk and read `d.Ops()` to learn how many appends there are (do not hard-code a number, because engines differ), then try every `At` with every `Kind` and every sensible `Keep` (`Short`: Zero, One, Half, AllButOne; `Lost`: just one run; `Crash`: all five).

## Limits

A checker only finds the failures it injects. This one tests single faults in the append path of one disk; it does not explore two faults in one run, failed reads, bit rot or concurrent access. Passing it means "no violation within this schedule", not "correct". The five broken engines are known cases, which is a weaker test than finding unknown bugs, and a hidden test of the correct engines guards against reporting false alarms.
