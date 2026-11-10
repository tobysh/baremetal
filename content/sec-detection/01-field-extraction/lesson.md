---
title: Extract fields from key=value logs
summary: Parse structured log lines whose values may be quoted, so rules can read fields instead of grepping text.
minutes: 25
difficulty: easy
tags: security, detection, parsing
id: sec-detection/field-extraction
mode: stdout
---
Every detection starts with parsing. Modern services log `key=value` pairs, and a value that contains spaces is wrapped in double quotes, with a backslash escaping a quote or backslash inside it. Rules, counters and allowlists later in this course all read fields, so a parser that is quietly wrong makes every rule built on it wrong too.

Searching for the text `user=` is the classic shortcut, and it goes wrong in three ways: `username=` also contains it, a quoted message such as `msg="user=root"` can contain a fake field, and the same key can appear twice. Here is one line that trips all three:

```text
ts=1700000294 username=deploy msg="user=root src=1.1.1.1" user=alice user=mallory
```

A grep for `user=` finds four candidates and picks the wrong one first. The correct answer is `user=alice`: `username` is a different key, the `user=root` sits inside a quoted value, and the later `user=mallory` must not override the first real `user`.

## The idea: one pass, left to right

A correct extractor walks the line once and keeps a position:

1. Skip spaces. Stop at the end of the line.
2. Read the key up to `=` or a space.
3. If the next character is `=`, read the value: after an opening `"` read to the closing `"`, treating `\"` and `\\` as literal characters; otherwise read up to the next space.
4. Compare the key against the wanted names as a whole (length and bytes), and keep only the first occurrence.

```text
input : action="read file" src= user=bob
output: user=bob src= action=read file
```

Because the value of `msg` is consumed as one unit in step 3, the text inside it can never be mistaken for a key. That is the whole defence against [log injection](/learn/sec-forensics/log-injection): never let data pretend to be structure.

## Pitfalls

- **Prefix matching.** `strncmp(k, "user", 4)` accepts `username`. Compare the length too.
- **Last-wins versus first-wins.** If an attacker controls a later value, last-wins lets them overwrite the real field. Take the first occurrence.
- **Empty versus absent.** `src=` is present with an empty value and prints as an empty string; no `src` at all prints `-`. They mean different things to a rule.
- **Escapes.** After a backslash inside quotes, copy the next character instead of treating it as the closing quote.

## Why defenders care

Detection quality is capped by parse quality. If the extractor misreads one field in a thousand, the rule that depends on it misses that attack or alerts on the wrong account, and nobody notices because no error is raised. The rest of this course builds on this parser, so later lessons give it to you ready made.
