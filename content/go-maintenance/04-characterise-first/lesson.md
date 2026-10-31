---
title: Characterise Before You Refactor
summary: Choose inputs that pin down a legacy phone normaliser so well that any one-line change to it would be noticed, before touching a line of it.
minutes: 35
difficulty: hard
tags: characterisation, testing, mutants
id: go-maintenance/characterise-first
mode: harness
---
Legacy code often has no tests, and you cannot tell whether a refactor kept behaviour unless something records what the behaviour **is**. A **characterisation test** does not say what the code should do; it records what it *does*, including the odd parts, so a later change that alters it is noticed.

A set of inputs is only as good as the changes it would catch. The way to measure that is **mutation**: make a small deliberate change to the code (a `>` turned into `>=`, a branch removed) and see whether your inputs notice. A mutant that nobody notices marks a behaviour you have not pinned.

## This lesson

The starter has `Normalize`, a legacy phone normaliser. You are not asked to change it. You are asked to write `Cases()`: a list of at most 30 input strings. The hidden harness holds the original function and **fourteen broken copies**, each with one small defect. Your inputs pass when every broken copy gives a different answer from the original on at least one of your inputs.

Read the function, think about each branch and each boundary, and pick inputs that sit on the edges: one just inside, one just outside.

## Honest limits

Killing these fourteen mutants does not prove your inputs pin the whole function: it proves they catch these fourteen changes. Real mutation testing generates hundreds of mutants automatically. The skill transfers; the guarantee does not.
