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
