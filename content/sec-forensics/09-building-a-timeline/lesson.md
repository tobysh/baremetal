---
title: Build a stable timeline
summary: Merge fictional timestamped events into UTC order while preserving ties and reporting quiet intervals.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/building-a-timeline
mode: stdout
---
## The forensic question

Three systems each tell part of a story: a web server log, a file system listing and an authentication log. No single one shows that an upload, a privilege escalation and a configuration change happened within minutes of each other. A **timeline** merges all events onto one clock so the sequence becomes visible.

## The data

Each input line is `UnixSeconds source message`. The message runs to the end of the line and may contain spaces. Here is a small synthetic input, deliberately out of order:

```text
1771247699 web GET /index.html 200
1771215549 web POST /login 302
1771202563 mtime modified /home/dana/notes.txt
1771202910 web GET /index.html 200
1771202910 auth sudo by dana: cat /etc/shadow
```

Sorted, with UTC dates, quiet-period markers and a summary, the report is:

```text
2026-02-16T00:42:43Z [mtime] modified /home/dana/notes.txt
2026-02-16T00:48:30Z [web] GET /index.html 200
2026-02-16T00:48:30Z [auth] sudo by dana: cat /etc/shadow
-- quiet for 3h 30m --
2026-02-16T04:19:09Z [web] POST /login 302
-- quiet for 8h 55m --
2026-02-16T13:14:59Z [web] GET /index.html 200
5 events over 12h 32m
```

## Worked example

1. Parse each line: the first number is the time, the second token is the source, and the rest (after the whitespace that follows the source) is the message. `sscanf` with a `%n` at the end tells you where the message begins; copy from there and strip the newline.
2. Sort by time. The two events at `1771202910` keep their input order (`web` before `auth`, as in the input): a **stable** sort. If you sort with an unstable method the order of ties may change between runs, and the report would not be reproducible.
3. Walk the sorted list. Before printing an event that is not the first, compute the gap to its predecessor. If the gap is at least 3600 seconds, print `-- quiet for Hh MMm --`: whole hours, then the remaining whole minutes with two digits. A gap of 3599 seconds prints nothing; exactly 3600 prints `1h 00m`.
4. Format the time as ISO-8601 UTC with `gmtime` and `strftime`, not `localtime`, so the output does not depend on the machine's time zone.
5. Finish with `N events over Hh MMm`: the count and the difference between the last and first time, hours unpadded.

## Pitfalls

- **Time zones.** Different sources log local time, UTC or epoch seconds; convert first. Here everything is epoch seconds, so only the display needs UTC.
- **Stability.** An insertion sort that shifts only strictly greater elements is stable.
- **Truncated or odd lines.** Real logs have them; this dataset does not, but a parser should skip a line it cannot read rather than guess.
- **Whole hours.** `7199` seconds is `1h 59m`, not `2h`.
- **Message spacing.** Do not collapse internal spaces.

## What a defender concludes

The timeline shows order and spacing, not causation. An upload followed by a configuration change twelve seconds later is a lead; a quiet gap only means these supplied sources recorded nothing then. Clock drift between hosts can reorder events that were close together, so say which clock each source used and keep ties marked rather than ranked.
