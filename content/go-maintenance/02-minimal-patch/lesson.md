---
title: The Minimal Patch
summary: Fix one bug in a title-casing helper without changing any other behaviour, including the quirks that look wrong but are relied on.
minutes: 30
difficulty: medium
tags: patching, regression, scope
id: go-maintenance/minimal-patch
mode: harness
---
A bug report is a small request: *this input gives the wrong output*. A good maintenance patch does exactly that and nothing else. Every extra behaviour you change is a surprise for a caller who depended on it.

Three habits make a patch minimal:

- **Locate the cause, not the symptom.** Find where the wrong output is produced.
- **Check for look-alikes.** Code that has the same shape in another place is not necessarily wrong there. A copy-paste idiom can be a bug in one function and a deliberate rule in the next.
- **List what must not change** before you edit, and check it afterwards.

## The report

> `Title("don't stop")` returns `Don'T Stop`. It should be `Don't Stop`.

The starter has a title-casing helper, a name-casing helper, and a few functions that call them. Both casing helpers use the same loop idiom. Only one of them is wrong.

## What a patch cannot tell you

The tests pin the behaviour listed in the task. Anything the tests do not pin is yours to reason about, which is the real situation in maintenance: no test suite covers everything.
