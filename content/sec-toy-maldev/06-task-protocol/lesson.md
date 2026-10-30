---
title: Validate an inert task protocol
summary: Process finite in-memory messages while rejecting replay, wrong-session data and malformed batches.
minutes: 40
difficulty: hard
tags: security, toy-maldev, task-protocol
id: sec-toy-maldev/task-protocol
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

A task protocol separates instructions from results. This lesson models that boundary as a JSON batch passed directly to a function. It has only echo and stop tasks; there is no remote endpoint, polling loop, shell command, file transfer or background agent.

A monotonically increasing sequence number can reject replay relative to caller-provided state. An exact session label prevents accidental cross-session mixing in the model. Neither is authentication: without a trusted transport and authenticated messages, a hostile sender can still forge fields. This lesson deliberately studies parsing and replay state without creating a real command channel.

Treat the batch transactionally. If its final task is invalid, the caller must not advance the replay counter or accept earlier replies. Reject trailing JSON so the same bytes cannot have an unnoticed second interpretation.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
