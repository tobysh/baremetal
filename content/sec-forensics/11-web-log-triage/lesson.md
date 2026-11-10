---
title: Triage a web access log
summary: Count status classes in a synthetic access log, set aside malformed lines and flag sources that repeatedly request traversal-style paths.
minutes: 30
difficulty: medium
tags: security, log-analysis, parsing
id: sec-forensics/web-log-triage
mode: stdout
---
## The forensic question

A web server writes one line per request. Which clients asked for paths that look like directory traversal, and how many lines in this log did not parse at all? The second question matters as much as the first: logs are partly attacker-controlled text, and a sudden rise in malformed lines is itself a finding.

## The data

Each synthetic line has the client address, other fields, a quoted request (`METHOD PATH VERSION`), a status code and a size (digits or `-`):

```text
203.0.113.9 - - [03/Mar/2026:02:18:30 +0000] "HEAD / HTTP/1.1" 301 -
192.0.2.10 - - [03/Mar/2026:02:46:04 +0000] "HEAD /index.html HTTP/1.1" 200 9177
198.51.100.140 - - [03/Mar/2026:02:39:28 +0000] "HEAD /a/%2E%2e/b HTTP/1.1" 200 5389
198.51.100.140 - - [03/Mar/2026:02:41:02 +0000] "GET /download?f=../../etc/passwd HTTP/1.1" 404 -
192.0.2.10 - - [03/Mar/2026:02:27:04 +0000] GET /about 200 5
```

## Worked example

Go through the five lines above.

1. A line is **well formed** when it has a first token, a pair of double quotes enclosing exactly three whitespace-separated words, and, after the closing quote, exactly two tokens: a status from 100 to 599 and a size that is `-` or digits. The last line has no quotes, so it is malformed.
2. The first four are well formed. Statuses: 301 counts as 3xx, 200 twice as 2xx, 404 as 4xx. The summary starts `2xx=2 3xx=1 4xx=1 5xx=0 malformed=1`. A 1xx line is well formed but not counted in any class.
3. For traversal, lowercase the path and look for `../`, `%2e%2e` or `/etc/passwd`. Line three matches `%2e%2e` (after lowercasing `%2E%2e`), line four matches `../` and `/etc/passwd` but is still a single request, so it counts once.
4. Count hits per address: `198.51.100.140` has two. An address is flagged only at two or more, because a single odd line proves little. Output:

```text
2xx=2 3xx=1 4xx=1 5xx=0 malformed=1
SUSPECT 198.51.100.140 hits=2
```

Flags are sorted by hits descending, then by `strcmp` on the address.

## Pitfalls

- **Count what you cannot parse.** Do not skip malformed lines silently; report them, and do not count their paths toward hits.
- **Empty lines are malformed**, as are lines with two or four words in the quotes.
- **Status range.** 099 and 600 are malformed. 100 and 599 are valid.
- **Case.** `%2E%2E` and `%2e%2e` are the same encoded `..`.
- **Crude match.** This rule also fires on `a/../b` harmless paths and misses `..%2f`. Real detection decodes first and looks at the response.
- **Bounds.** A hostile line can be very long; read into a fixed buffer with `fgets` and parse with width limits.

## What a defender concludes

A flagged address made repeated requests whose paths look like traversal attempts, and the log does not tell you whether any succeeded (a 404 is the ordinary outcome of a probe). Shared addresses and forged logs affect attribution. Report the counts, the status classes and the malformed total, then pull the matching lines for a human to read.
