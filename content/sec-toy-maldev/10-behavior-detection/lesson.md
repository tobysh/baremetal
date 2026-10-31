---
title: Detect behavior in interpreter traces
summary: Write three trace detectors, then test them against benign look-alikes, misleading strings and jitter, and read a versioned findings report.
minutes: 55
difficulty: hard
tags: security, toy-maldev, detection, false-positives
id: sec-toy-maldev/behavior-detection
mode: harness
---

This course constructs **inert payload data** for an invented machine. Payload instructions can emit text, read explicitly supplied map entries or advance virtual time. The toy machine has no instruction for host execution, network access, process injection or filesystem mutation. Everything stays inside the existing Go sandbox.

So far you built programs. Defenders look at what programs **do**. A static signature asks whether the bytes look like something known; a behavior detector asks what happened during a run. Behavior is harder to disguise cheaply, but every detector has two error modes: it flags harmless programs (false positive) and misses harmful ones (false negative). A detector is only as good as the benign cases you tried to break it with.

The machine records an ordered **trace** of events. The starter supplies `Execute`, which turns a program and its capability map into events:

| Kind | Text | Ticks |
| --- | --- | --- |
| `read` | the capability key that was read | 0 |
| `emit` | the emitted text | 0 |
| `wait` | empty | the virtual ticks waited |

## The three detectors

1. `secret-read` (severity `high`): a `read` event whose key starts with the path prefix `secret/`. The match is on the key of a read, not on any text that merely contains the word.
2. `periodic-beacon` (`medium`): emitting the same text at regular, nonzero intervals. Take all `emit` events with one text, in order; the gap between two of them is the sum of `wait` ticks between them. The first time three consecutive same-text emits have two equal gaps greater than zero, report the first of the three, once per text. Operators can list texts as expected heartbeats in the policy, which suppresses this rule for exactly those texts.
3. `delayed-start` (`low`): a `read` or `emit` event happens only after more than `DelayTicks` virtual ticks of waiting.

Findings are sorted by event index, then by ID.

## What the detectors get wrong

State these limits in your own words after the tests pass. The rules are guesses about intent, not proof of it:

- A health-check program that prints `ping` every 30 ticks is a periodic beacon by definition. The policy allow-list is how a human records "this one is expected". Suppression must be as narrow as the evidence: it must not hide a `secret-read`, and a similar text such as `ping2` is not covered.
- A real adversary adds random jitter and the equal-gap rule misses it. This is a false negative the tests deliberately include, and it shows why a rule is not a guarantee.
- Searching emitted text for `secret/` would flag harmless documentation and miss a read that happens through a different spelling. The rule above avoids the first problem and has its own blind spots: a key outside `secret/` is never flagged, however sensitive.
- A long wait can be a scheduled job. `delayed-start` is low severity because it is weak evidence on its own.

## Versioned reports

Findings are stored as JSON. Schema 1 had `id` and `index` only; severity was implied by the ID. Schema 2 adds an explicit `severity`. `ParseReport` must read both, fill in the severity for schema 1, require severity in schema 2 and make sure it matches the ID's severity, and reject unknown schemas, unknown fields, unknown IDs and trailing data. Old stored reports stay readable; new readers do not trust fields they do not understand.

Detection is not enforcement. Everything here runs on a trace that was already produced, in memory.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
