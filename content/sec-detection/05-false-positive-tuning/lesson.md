---
title: Tune alerts with an allowlist
summary: Suppress known-good alerts with wildcard patterns, count what each entry suppresses and find stale entries.
minutes: 35
difficulty: medium
tags: security, detection, tuning
id: sec-detection/false-positive-tuning
mode: stdout
---
A rule that fires constantly on known-good activity trains analysts to ignore it. Tuning is how you fix that without losing the rule: add narrow exceptions for the monitoring scanner, the backup account or the lab subnet, and review them regularly. The goal is fewer alerts that are all worth reading, not fewer alerts.

## Globs

Exceptions are written as patterns. Here `*` matches any run of characters (including none) and `?` matches exactly one. Each allow entry has three globs, one for the rule name, the source and the user, and an alert is suppressed by the **first** entry where all three match.

```text
ALLOW brute-force 192.0.2.5 scanner
ALLOW brute-force 10.9.*   *
ALLOW ?ync-*      *        backup
(blank line)
brute-force 192.0.2.5   scanner
brute-force 10.9.3.4    alice
brute-force 203.0.113.8 alice
sync-job    198.51.100.1 backup
```

```text
kept=1 suppressed=3
allow 1 hits=1
allow 2 hits=1
allow 3 hits=1
kept: brute-force 203.0.113.8 alice
```

Only the third alert survives: the others match entry 1, entry 2 and entry 3 respectively. If an entry matches nothing, its report line gets ` stale` appended.

## Matching with recursion

Wildcard matching is naturally recursive. If the pattern is empty, the text must be empty too. When the pattern starts with `*`, try matching the rest of the pattern at every position of the remaining text, including the end. When it starts with `?`, consume one text character (there must be one). Otherwise the characters must be equal, and you recurse on both tails. A trailing `*` therefore matches the empty string, and `a*` matches `a`.

## Pitfalls

- **Broad beats the whole rule.** An entry like `* * *` silently disables the detection, which is why the report says how many alerts each entry removed.
- **First match wins, once.** When several entries would match, only the first counts a hit. Later entries that are always shadowed show up as stale.
- **Stale entries.** An entry that has removed nothing for a long time is obsolete or hides a typo, and an unused allowlist entry is a small standing hole.
- **`?` needs a character.** `?` does not match an empty string, so `ab?` does not match `ab`.
- **Order of output.** Kept alerts keep their input order; allow lines are reported in their own order.

## Why defenders care

Narrow beats broad, and every exception should have an owner and a review date. Counting hits turns the allowlist into something you can audit. You used literal matching in [rule matching](/learn/sec-detection/rule-matching); this lesson adds globs, and the [review pipeline](/learn/sec-detection/review-detection-pipeline) combines them with a window.
