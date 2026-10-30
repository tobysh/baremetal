---
draft: true
title: Schedule work on a virtual clock
summary: Model delays, retries and cancellation with a fake clock and a deterministic event order, with no timers and no background work.
minutes: 40
difficulty: hard
tags: security, toy-maldev, scheduling, determinism
id: sec-toy-maldev/virtual-scheduling
mode: harness
---

This course constructs **inert payload data** for an invented machine. Nothing here sleeps, spawns a process, opens a connection or keeps running after a function returns. Everything stays inside the existing Go sandbox.

Real programs wait: they delay a first attempt, retry after a failure and give up when told to stop. Code that calls `time.Sleep` or starts a goroutine to do this is slow to test and different on every run. The WAIT instruction in this track already points at a better design: time is just a number the caller owns.

In this lesson time is a `uint64` of virtual ticks. Your scheduler is a pure function. It receives a list of simulated jobs and a horizon, and returns the ordered list of events that would happen. It never reads the real clock, never sleeps and never starts anything in the background. A test that simulates four billion ticks must finish instantly, so jump from one event to the next instead of counting ticks.

Determinism is the property to protect. When two jobs are due at the same tick, the one with the smaller ID goes first, whatever order the caller listed them in. A job whose cancellation tick has been reached never starts an attempt, even if the attempt is due at the same tick.

Retries use exponential backoff, which spreads attempts out. After attempt `n` fails, the next attempt is due `Backoff << (n-1)` ticks later. Validate every field before simulating anything, because a hostile job list should be rejected rather than half run.

Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
