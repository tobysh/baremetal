---
title: "Transfer: Rises in a Series"
summary: Solve a small problem from a written description alone, with no signature, no main and no list of edge cases, using only what the course taught.
minutes: 30
difficulty: medium
tags: transfer, input, loops
id: first-steps/transfer-rises
mode: stdout
---
Every lesson so far gave you a starting shape: a function to fill in, or a `main` with a comment inside it. This one gives you a description and an empty file. Nothing here is new. It is the same loops, variables and `scanf` you have already used, so the only question is whether you can put them together yourself.

## Work through the recipe

Use the six steps from the earlier lessons, in order.

1. **Understand the problem.** Say what goes in and what comes out, in your own words, before you write any code.
2. **Write examples.** Pick two or three inputs and work out the output by hand. Include an awkward one: a series with nothing in it, or one that only ever goes down.
3. **Choose signatures.** You may write helper functions, or put everything in `main`. Either is fine.
4. **Plan.** Which patterns do you need? This task is a **sentinel loop** (read until the input ends) around a **counter** and an **accumulator** that tracks a biggest value. Write the steps as comments first.
5. **Code.** Fill the comments in one at a time.
6. **Test.** Press Run with your own examples. When the answer is wrong, add a `printf` that shows the variable you suspect, and check your guess against the output.

## One thing to decide first

To compare a number with the one before it, you have to *remember* the one before it. That is an ordinary variable that you update at the end of every pass of the loop. Decide what it holds on the very first pass, when there is no number before.

Hints are available if you get stuck. Using them is recorded, as in every lesson.
