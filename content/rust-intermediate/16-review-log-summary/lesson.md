---
title: "Review: Parallel Log Summary"
summary: A consolidation exercise. Count log levels in parallel with a lifetime helper, an error enum, worker threads, a shared Mutex map and a channel. No new ideas.
minutes: 30
difficulty: medium
kind: review
tags: threads, errors, lifetimes, channels
id: rust-intermediate/review-log-summary
mode: harness
---
This is a **review lesson**: no new concepts, but all four parts of the course in one small program. It uses [lifetimes](/learn/rust-intermediate/lifetime-annotations), [custom errors](/learn/rust-intermediate/custom-error-types), [channels](/learn/rust-intermediate/channel-pipelines) and [Arc and Mutex](/learn/rust-intermediate/shared-bank) together with [joining every thread](/learn/rust-intermediate/thread-results).

## The program

A log has lines such as `WARN disk almost full`. The first word is the level: `INFO`, `WARN` or `ERROR`. `summarize` splits the lines among worker threads. Each worker:

- finds each line's level with `level_of`, which returns a `&str` borrowed from the line,
- adds it to a shared `Arc<Mutex<BTreeMap<String, usize>>>` of counts,
- reports any bad line by sending its line number over a channel.

After joining the workers, `summarize` returns the counts, or `LogError::BadLine(n)` for the **lowest** bad line number, so the answer does not depend on thread timing.

## Design notes

- Split the lines with `lines.chunks(size)`, where `size` is `ceil(len / workers)` and at least 1. A worker needs its own owned copy of its chunk (`chunk.to_vec()`) and the number of its first line.
- Keep each lock short: lock, update the map, release. Never hold the lock while sending.
- Drop the original `Sender`, then join the workers, then read the channel.
- Line numbers start at 1.
