---
title: Measured Slow Path
summary: Stop guessing where a program is slow. Read per-stage operation counters, fix the stage that dominates, and prove the gain with counts instead of a stopwatch.
minutes: 25
difficulty: medium
tags: debugging, performance, profiling, evidence
id: c-debugging/measured-slow-path
mode: harness
---
"This is slow" is a symptom, not a location. Intuition about *where* is wrong often enough that the first move is to **measure**.

## The program

`word_freq` runs three stages over a text:

1. **tokenize** splits the text and `malloc`s a copy of every word.
2. **count** finds the distinct words and counts them.
3. **render** writes `word=count` lines.

The program keeps operation counters per stage: comparisons of words, allocations, and bytes copied. Running it on 2000 words (400 possible values) prints this profile:

```text
397 distinct words
stage       compares   allocs   copied
tokenize           0     2000     9469
count         362069        0        0
render             0        0     2680
```

## Reading the table

The stage that *looks* suspicious is `tokenize`: 2000 separate `malloc` calls, one per word. Allocation has a reputation for being slow, so it is tempting to start there.

The numbers say otherwise. `tokenize` does 2000 allocations and copies 9 KB. `count` does **362 069 comparisons**, about 180 per word, because each word is compared with the distinct words seen so far until it matches or the list ends. One stage dominates by two orders of magnitude, and rewriting the others could not move the total.

Two habits come out of this:

- Rank stages by their counters before touching code.
- Predict the change. If lookups use a hash table, comparisons per word should drop from "number of distinct words" to "length of a short chain", roughly 1 to 2.

## Why counters are the oracle

A stopwatch result depends on the machine, the other programs running, the CPU frequency, caches, and whether the run was the first or the tenth. A test that says "finished in under 50 ms" can fail on a busy laptop and pass on a fast one with the slow algorithm still in place. A timing check alone **does not prove a performance claim**.

A counter is different. It counts a defined event, such as one word comparison, and gives the same value on every run. It tells you *why* the work changed, not just that something moved. That is why the tests here read the counters, together with a correctness check: the output must equal a reference result computed separately, because a program that skips work will also have excellent counters.

Counters have limits too. They count only what you chose to instrument, and a bad choice hides the real cost. Use them for a deterministic judgement, and time the final result as a sanity check, not as proof.

## Your turn

Fix the dominant stage. Keep the output identical and keep routing every word comparison through `same`, so the counters stay honest.
