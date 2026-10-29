---
title: "Review: Word Census"
summary: A consolidation exercise. Combine spawn and join, move closures, a channel and a shared Arc<Mutex> counter to count words across worker threads. No new ideas.
minutes: 25
difficulty: medium
kind: review
tags: threads, channels, mutex
id: rust-concurrency/review-word-census
mode: harness
---
This review combines [threads](/learn/rust-concurrency/spawn-and-join), [channels](/learn/rust-concurrency/channels) and [shared locking](/learn/rust-concurrency/arc-mutex).

This is a **review lesson**. Nothing here is new: you have spawned threads, moved data into closures, passed messages over channels and shared a counter behind `Arc<Mutex<_>>`. Now use them together.

The job is a *census*: given many lines of text, count how often each word occurs. The plan:

1. Deal the lines out to `workers` threads, round-robin: line `i` goes to worker `i % workers`.
2. Each worker counts the words of its own lines into a **local** `HashMap`. Local data needs no lock at all, which is the cheapest kind of synchronisation.
3. Each worker **sends** its finished map over a channel. The calling thread merges the maps into one `BTreeMap` (sorted keys give deterministic output).
4. All workers also add the number of words they saw to one shared total in an `Arc<Mutex<usize>>`.

Two small twists to remember:

- Words are compared **case-insensitively**: `Rust` and `rust` are the same word. Split on whitespace and use `to_lowercase()`.
- Treat `workers == 0` as `1`. A worker that gets no lines should still be fine.

And one old trap: the receiving loop only finishes once every sender is dropped, including the original you cloned from.
