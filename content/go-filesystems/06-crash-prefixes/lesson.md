---
title: Enumerate Every Crash Point
summary: Replay prefixes of atomic block writes and identify inconsistent durable states.
minutes: 40
difficulty: hard
id: go-filesystems/crash-prefixes
---
A crash can preserve only part of a multi-write operation. Our model writes whole blocks atomically and in issue order: a cut after k writes retains exactly the first k writes. It does not model torn blocks, reordering or volatile device caches. Enumerating every prefix makes the safety claim precise rather than relying on a random lucky crash.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
