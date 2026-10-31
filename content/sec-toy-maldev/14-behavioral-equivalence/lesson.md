---
title: Prove two packagings behave the same
summary: Compare differently packaged artifacts by canonical behavior traces over scenarios, and learn what "the same" can and cannot mean.
minutes: 45
difficulty: hard
tags: security, toy-maldev, equivalence, traces
id: sec-toy-maldev/behavioral-equivalence
mode: harness
---

This course constructs **inert payload data** for an invented machine. Instructions emit text, read caller-supplied map entries or advance virtual time; nothing executes natively. Everything stays inside the existing Go sandbox.

The same behavior can wear many costumes. Two bundles with different XOR keys decode to the same capsule. Two different capsules can differ in code that is never reached, or split `WAIT 5` into `WAIT 2; WAIT 3`. Byte-level comparison calls all of these different, and signature-style matching on bytes is brittle for the same reason. Analysts therefore compare **behavior**: run both under the same conditions and compare what was observed.

Behavior here means the trace: outputs in order, the keys read, and the final tick count. To compare traces cheaply, reduce each to a **canonical fingerprint**. Canonical means two equal traces always give the same string and two different traces never do. Concatenating outputs fails the second rule: `["a","bc"]` and `["ab","c"]` would collide. Length-prefix every field.

Equivalence is always **relative to scenarios**. A scenario is a capability map plus budgets. Two programs may agree when a key is missing and differ when it is present, and a split WAIT may behave identically until the operation budget is tight. Rejection is also behavior: two artifacts that both fail under a scenario agree there, but one that fails and one that succeeds differ. Error message text is not compared.

A finite set of scenarios can never prove equivalence in general; it can only fail to find a difference. Say so in your conclusions. Comparing zero scenarios proves nothing and is reported as an error.

The loader, interpreter and packaging helpers are provided. Run uses a small public example in main. Test exercises fresh values and malformed inputs. Keep the provided API names so the harness can call your functions.
