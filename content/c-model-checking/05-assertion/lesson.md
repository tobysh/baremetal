---
title: Break the Invariant
summary: Check an invariant after every step, so a violation is caught even when a later step repairs it or no final-state check would notice.
minutes: 35
difficulty: medium
tags: model-checking, invariant, check-then-act
id: c-model-checking/assertion
mode: harness
---
A final-state check cannot see a problem that comes and goes. An **invariant** is a property that must hold in *every* state, so it is checked after every step.

A `program` has one: `m[inv_var]` must stay within `inv_lo..inv_hi` (`inv_var = -1` disables it). Violating it is `V_ASSERT`, the model's version of a failed `assert`.

## Check-then-act

An account holds 100 and two threads each withdraw 60:

```text
LOAD  r0, balance
if r0 < 60: done
LOAD  r0, balance      ; act on a fresh read
ADDI  r0, -60
STORE balance, r0
```

Each thread checks before it acts, but another thread can act in between. If both pass the check, the second writes `-20`. Holding one lock across the check *and* the act makes the pair atomic.

Another case: a thread that briefly writes `-50` and then writes `100` again never ends in a bad final state, yet another thread could read the `-50`. The invariant catches it because it looks at every state.

The initial state counts as well, with depth 0.
