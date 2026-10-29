---
title: Detect truncated output
summary: Build user@host in a fixed buffer, and refuse rather than leave a silently shortened, wrong identity behind.
minutes: 20
difficulty: easy
tags: security, snprintf, truncation
id: sec-c-bugs/truncated-output
mode: harness
---
`snprintf` never writes past the buffer, which makes it much safer than `sprintf`. It does not make a short buffer harmless, though. When the text does not fit, `snprintf` cuts it and still reports success in its own way: the return value is the length it would have needed, and a caller who ignores that value keeps a silently shortened string.

Truncation is a security bug when the shortened text means something else. If `alice@build.example.test` is cut to `alice@build`, an access check may now compare against a different, valid host name. A truncated path can name a parent directory, and a truncated command can run a different program. The remedy is to compare the returned length with the capacity and treat any overflow as an error, setting the output to an empty string so later code cannot use the fragment.

In this lesson `build_label` joins a user and a host with `@`. Decide whether it fits before you write, or use `snprintf` and inspect its result. Either way, a failure must leave `out` empty (when `cap` is at least one) and a zero capacity must leave memory untouched. Names are synthetic.
