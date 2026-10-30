---
draft: true
title: Recover from crashes and tear down cleanly
summary: Journal every simulated state change before making it, so any interruption can be recovered and the host audited back to its baseline.
minutes: 45
difficulty: hard
tags: security, toy-maldev, teardown, crash-recovery
id: sec-toy-maldev/crash-recovery-teardown
mode: harness
---

This course constructs **inert payload data** for an invented machine. The "host" in this lesson is three Go maps in memory: a fake file table, a fake settings table and a fake task table. Nothing touches the real filesystem, registry, scheduler or network. Everything stays inside the existing Go sandbox.

Any software that changes a system takes on a duty to undo the change: a test lab that is left dirty after a run teaches the wrong habits, and a lab that cannot be reset is a liability. The hard case is the interrupted run. Power fails, a process is killed, an error occurs half way. What is left behind?

The classic answer is a **write-ahead journal**. Before changing anything, record what the thing looked like beforehand: whether it existed and what its old value was. Only then make the change. If the run stops at any moment, the journal says exactly what to restore, including the case where a change was recorded but never made.

Recovery walks the journal **backwards**. Doing so matters because the same name can be written twice in one run, and each entry records the state before its own write. Restoring newest to oldest ends on the true original. Recovery should be **idempotent**: a second call after a successful one must change nothing, so that a crash during cleanup can simply be retried.

The final tool is an **audit**: compare the host with a baseline snapshot and list every difference. A teardown that has not been audited is only a belief that it worked.

The tests interrupt a run at every point in the lifecycle, restart by copying the journal as a restarted process would load it, recover, and audit. Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
