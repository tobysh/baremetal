---
title: "Capstone: a toy update client that survives failures"
summary: Verify a signed package, install it into an A/B slot layout with one atomic commit point, and roll back safely when the disk or the new version fails.
minutes: 75
difficulty: hard
tags: security, updates, rollback, fault-injection, capstone
id: sec-secure-delivery/update-client
mode: harness
---
This capstone joins the course: digests and signatures (lessons 1 to 3), freshness and downgrade rules (lesson 7) and the idea that anything stored must survive interruption. Power fails, disks lie, new versions turn out broken. An update client has one non-negotiable job: **after any failure the device still boots a complete, verified version, and an attacker can never use the failure to install something older.**

Everything here is a simulation. The "disk" is an in-memory map with a numbered list of operations and an injected **failure schedule**; the keys are throwaway values from fixed seeds. The simulation demonstrates the stated fault model and nothing more: a real flash device has write-reordering, cache and wear behaviours that a toy `Disk` does not model.

## Layout (all supplied)

- Two slots, `slot.a` and `slot.b`. One is running, the other is a spare.
- `active` says which slot is current (`cur`) and which previous good version is kept (`prev`), each with counter, issue time, version and digest.
- `hw` is the high-water mark from lesson 7: the highest counter ever committed and the newest issue time.
- `active` and `hw` are only replaced with `atomicWrite` (write a temp file, read it back, rename it over the target). `Disk.Write` can tear; `Disk.Rename` cannot. **The rename of `active` is the commit point.** Before it, the old version is still the one that boots; after it, the new one is.

## The faults

`Disk.Plan` maps an operation number to one of: `FaultError` (fails, no effect), `FaultTorn` (half written, error), `FaultCrash` (process dies before the op), `FaultTornCrash`, `FaultCrashAfter` (op happens, then the process dies) and `FaultLie` (half written but reports success). After a crash every call fails until `Restart`, which models the process starting again with the disk exactly as it was left.

## What you write

- `Verify`: signature, metadata, freshness (`Decide`), then the body's size and digest. No side effects.
- `Install`: the ordered steps in the doc comment. Think about why each comes where it does: verify before any write; write the spare slot, never the running one; read back what you wrote; commit with one atomic step; only then raise the high-water mark (raising it first would make a retry of a failed install look like a downgrade); run the health check last.
- `Recover`: clean up after an interrupted run, idempotently.
- `Rollback`: raise the high-water mark *before* switching back, so the broken version can never be installed again.
- `Boot`: trust nothing on disk; check the digest and fall back if the running slot is damaged.

## Properties the tests check after every failure schedule

1. `Boot` returns exactly the old or the new version, never a torn mixture.
2. If `Install` returned nil, the new version boots.
3. The high-water mark never decreases and is never below the booted counter.
4. After `Restart` and `Recover`, retrying the same package eventually succeeds (or reports it is already installed).
5. Once the new version is installed, an old signed package is refused with `ErrDowngrade`, and **a refused package performs no disk writes at all**.

What the tests cannot establish: they enumerate every single fault position, and thousands of random combinations, but not all schedules. They also cannot judge whether your ordering is the *simplest* correct one. That is a design discussion; compare your solution with the one the reference makes.
