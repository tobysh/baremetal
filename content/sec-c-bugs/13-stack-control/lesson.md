---
title: Redirect a modeled stack return
summary: Craft binary input that crosses a stack buffer boundary and changes a modeled return target, then repair the copy.
minutes: 40
difficulty: hard
tags: security, stack, control-flow
id: sec-c-bugs/stack-control
mode: harness
---

A classic stack overflow writes beyond a local buffer into saved control data. After a function returns, the processor may resume at the overwritten address. The payload depends on the layout, byte order and target address; embedded zero bytes are data, so string functions are unsuitable.

This lesson uses a deterministic byte-array stack-frame model rather than invoking C undefined behavior or disabling host protections. The first `offset` bytes model the local buffer; the next eight model the saved return address in little-endian order. The toy vulnerable copy accepts bytes across that logical boundary. A dispatcher grants the fictional win only if those eight bytes decode to the supplied win address. The harness never executes an address from learner data.

You write a payload for several layouts and addresses, then repair the copy boundary. This demonstrates the overwrite and its fix without relying on a particular compiler's stack layout. Native return-address exploitation, stack canaries, ASLR, NX and ROP are outside the exercise. No mitigations are weakened in the runner. Continue to the [synthetic infrastructure labs](/learn/sec-synthetic-labs/scan-correlation) for evidence analysis.
