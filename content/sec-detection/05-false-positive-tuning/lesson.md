---
title: Tune alerts with an allowlist
summary: Suppress known-good alerts with wildcard patterns, count what each entry suppresses and find stale entries.
minutes: 35
difficulty: medium
tags: security, detection, tuning
id: sec-detection/false-positive-tuning
mode: stdout
---
A rule that fires constantly on known-good activity trains analysts to ignore it. Tuning is how you fix that without losing the rule: add narrow exceptions for the monitoring scanner, the backup account or the lab subnet, and review them regularly.

Exceptions are written as patterns. Here `*` matches any run of characters (including none) and `?` matches exactly one. The first matching exception suppresses the alert. Narrow beats broad: an exception like `* * *` silently disables the whole detection, so a good report states how many alerts each exception removed. An exception that has removed nothing for a long time is stale; it is either obsolete or hiding a typo, and an unused allowlist entry is a small standing hole.

Wildcard matching is naturally recursive: when you see `*`, try matching the rest of the pattern at every position of the remaining text. You used literal matching in [rule matching](/learn/sec-detection/rule-matching); this lesson adds globs.
