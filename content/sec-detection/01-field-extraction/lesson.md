---
title: Extract fields from key=value logs
summary: Parse structured log lines whose values may be quoted, so rules can read fields instead of grepping text.
minutes: 25
difficulty: easy
tags: security, detection, parsing
id: sec-detection/field-extraction
mode: stdout
---
Every detection starts with parsing. Modern services log `key=value` pairs, and a value that contains spaces is wrapped in double quotes, with a backslash escaping a quote or backslash inside it. Searching for the text `user=` is the classic shortcut, and it goes wrong in three ways: `username=` also contains it, a quoted message such as `msg="user=root"` can contain a fake field, and the same key can appear twice.

A correct extractor walks the line left to right, reads a key up to `=` or a space, then reads either a quoted value or a bare value up to the next space. Only a whole-key comparison counts, and the first occurrence of a key wins, because an attacker who controls one value should not be able to append a second `user=` later in the line and override the real one. This is the same discipline as [log injection](/learn/sec-forensics/log-injection): never let data pretend to be structure.

The rest of this course builds on this parser, so later lessons give it to you ready made.
