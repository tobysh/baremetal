---
title: Review Against a Defect Inventory
summary: Review a credit-pool service, identify defective regions, prove them with counterexamples, and preserve correct behavior.
minutes: 45
difficulty: hard
tags: code-review, counterexamples, specification
id: go-maintenance/code-review
mode: harness
---
A review comment that says "this looks wrong" is cheap. A useful one says **which behaviour** is wrong and **shows an input** where the code disagrees with its specification. Reviewers also have to avoid the opposite error: flagging code that is odd-looking but correct. Both errors cost the team time.

The starter is a credit pool with a written specification (see the task). Its code is divided into regions marked `[A]` to `[H]`. The author planted defects in some regions; the others are decoys that look suspicious and are correct. This is the **defect inventory**, authored in the hidden tests.

You return three things:

1. `Found()`: the ids of the regions that contain a defect.
2. `Counterexample(id)`: for each, a short list of operations on which the code gives a different observable result from the specification. The harness runs your list against a copy of the specification with **only that one defect** switched on, so a counterexample must really expose that defect.
3. The fixed `Pool`, which must agree with the specification on hundreds of generated operation sequences. Fixing a decoy region by "improving" it will break that agreement.

## Honest limits

The tests grade the ids, the counterexamples and the behaviour. They cannot grade whether you could explain *why* each defect matters or how you would have found it without the labels. Write that reasoning down for yourself before you look at the hints.
