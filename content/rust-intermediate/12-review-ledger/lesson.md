---
title: "Review: Ledger Replay"
summary: A consolidation exercise. Combine an error enum, Display, source(), ? and map_err to replay a small account ledger and report problems precisely. No new ideas.
minutes: 30
difficulty: medium
kind: review
tags: errors, collections, display
id: rust-intermediate/review-ledger
mode: harness
---
This is a **review lesson**. It brings together [custom error types](/learn/rust-intermediate/custom-error-types), [`?` and From](/learn/rust-intermediate/from-and-question-mark), [sources](/learn/rust-intermediate/boxed-errors-and-sources) and [Result](/learn/rust-intermediate/result-and-question-mark). Nothing new.

## The task in one picture

A ledger is text, one change per line:

```
alice:+50
bob:+20
alice:-30
```

`apply` replays it into a `BTreeMap<String, i64>` (a map that keeps its keys sorted, so output order is predictable). Three things can go wrong, so there are three error variants:

- the line is not `name:amount` (`Syntax`),
- the amount is not a number (`Amount`, which keeps the `ParseIntError` as its **source**),
- an account would go below zero (`Overdraft`).

## Reminders

- `map.entry(name).or_insert(0)` gives a mutable reference to the balance, creating it at 0 if needed. Or use `*map.get(name).unwrap_or(&0)` to read and `insert` to write.
- `"+50".parse::<i64>()` accepts a leading `+`.
- Check the overdraft **before** committing the change, and report the balance the account would have reached.
