---
title: Establish and remove modeled service persistence
summary: Create an enabled root service, simulate a reboot, verify the service effect and clean up the persistent definition.
minutes: 40
difficulty: hard
tags: security, offensive-lab, service-persistence
id: sec-infra-offense/service-persistence
mode: harness
---

All targets in this track are fictional in-process state machines. `Lab.Send` is a callback, not a network client. Only use the port(s) supplied by the exercise. No real programs, accounts, services, credentials or files are touched. Status codes resemble HTTP for readability but describe the toy protocol only.

A privileged service definition can make code run again after restart. The important distinction is between a service currently running and a persistent configuration enabled for future boot. Deleting a definition does not necessarily stop an already running instance.

The model has a unit store, an enabled flag, a running flag and a reboot operation. Its command `emit <marker>` records a proof in memory when the service starts. It is a toy DSL, not a systemd unit or a host command. A reboot resets the proof and restarts enabled units, making persistence observable. Stop the service and remove its definition, then verify that another reboot does not recreate the effect. Audit visibility remains available throughout.


## The model

```text
unit definition (stored)      Enabled: true   <- survives reboot
running instance (memory)     Running: true   <- lost on reboot

create PUT   -> stored, Enabled true, Running false
POST /reboot -> every enabled unit starts: Running true, "emit <marker>" records the proof
GET /service-proof -> the marker                  <- persistence shown
```

Two facts are easy to confuse: `Enabled` is configuration that decides what starts next boot, `Running` is the current process. Persistence is the first; the second is only today's state. That is why the lab makes you reboot before it believes you.

## A worked example

Unit `lab-unit-4`, marker `LAB-PROOF:boot-4`:

```text
PUT  /units/lab-unit-4   {Name, RunAs:"root", Command:"emit LAB-PROOF:boot-4", Enabled:true}   -> 201
POST /reboot
GET  /units/lab-unit-4   -> Running true, Enabled true, RunAs root, same Command
GET  /service-proof      -> 200 "LAB-PROOF:boot-4"          persistence confirmed
POST /units/lab-unit-4/stop
DELETE /units/lab-unit-4
GET  /units/lab-unit-4   -> 404
POST /reboot
GET  /service-proof      -> 200 ""                          nothing comes back
```

The second reboot is the real test of cleanup: if the proof returns again, something still re-creates the effect. The tests include a lab where that happens.

## Pitfalls

- Reading the unit before the reboot. `Running` is false at first, so the check would fail or pass for the wrong reason.
- Deleting without stopping. The definition goes, but an instance already running does not, and the model refuses a delete while it runs.
- Treating `204` or `404` as the end of cleanup. Reboot again and require an empty proof.
- Returning early when `/service-proof` fails. If the unit was created, stop and delete it anyway.
- Matching only part of the unit. Compare name, runner, command, enabled and running.

## The defender's half

Audit enabled units, not only running ones, and alert on new root-run definitions and on commands outside an allow-list. Compare the set of enabled units with what configuration management says should exist. After removal, reboot a test system and look again, because that is the only check which proves no second path starts it.
