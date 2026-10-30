---
title: Read Before You Change
summary: Predict what an unfamiliar shipping-quote codebase does, from reading alone, before you edit anything. Answers are encoded as code.
minutes: 25
difficulty: easy
tags: reading, maintenance, characterisation
id: go-maintenance/read-before-you-change
mode: harness
---
Before you change code you did not write, find out what it already does. Reading is a skill with a method:

1. **Find the entry point** (here `Quote`) and follow one input all the way through.
2. **Note the order of things.** When rules are tried in a list, the first match wins, so a later rule can be unreachable for some inputs.
3. **Track every adjustment** applied after the rule: surcharges, discounts, clamps. The order in which they run changes the result because integer division rounds down.
4. **Distrust names.** A rule called `heavy` is not necessarily the one that prices a heavy parcel.

The starter is a 100-line shipping-quote module. Do not change `Quote`. Read it, then fill in `Answer()` with what you believe each of the six questions evaluates to.

## Honest limits

You are allowed to run the code, and it is a fine way to check a reading. The grader can only check your answers, not how you got them, so this lesson is about building the habit. The second thing the tests check is that you left `Quote` alone: edits to code you were only asked to read are exactly how a maintenance task goes wrong.
