---
title: "Review: Log Analyzer"
summary: A consolidation exercise. Parse log lines, count them in a BTreeMap and tidy the results with iterators and Option combinators. No new ideas.
minutes: 25
difficulty: medium
kind: review
tags: review, strings, btreemap, iterators, option
id: rust-bridge/review-log-analyzer
mode: harness
---
This review combines [string methods](/learn/rust-bridge/strings-and-chars), [Option combinators](/learn/rust-bridge/option-combinators), [iterator adaptors](/learn/rust-bridge/iterator-adaptors) and [ordered maps](/learn/rust-bridge/btreemap-and-btreeset).

A log line looks like this:

```
ERROR net: connection lost after 30 s
```

The first word is the **level**, the second word ends with a colon and is the **module**, and the rest is the **message**. Anything that does not fit that shape is a bad line, and a good analyser reports it instead of crashing.

You will parse single lines into a struct, then summarise a whole log. The twist is small: some lines have extra spaces, and levels are case-insensitive.
