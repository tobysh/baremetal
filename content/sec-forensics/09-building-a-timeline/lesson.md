---
title: Build a stable timeline
summary: Merge fictional timestamped events into UTC order while preserving ties and reporting quiet intervals.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/building-a-timeline
mode: stdout
---
A timeline puts observations from different sources on one clock. It can expose a sequence that no single log makes clear, but clock drift, missing records and timezone assumptions still limit the conclusion. Here timestamps are already integer Unix seconds and all displayed times are UTC.

Each input line is `timestamp source message`, with the message continuing to the end of the line. Preserve spaces in that message. Sort by timestamp using a stable method: two events at the same second must retain their input order. An insertion sort is adequate for the bounded teaching dataset, or attach an original index as a tie-breaker.

Before each event after the first, compare it with the previous event. A gap of at least 3600 seconds gets a quiet-period line in whole hours and remaining whole minutes. Finally report the event count and the total span from earliest to latest. A quiet gap means only that these supplied records contain no event; it does not establish that nothing happened. The generator deliberately mixes sources and inserts ties so hardcoded output and unstable ordering both fail.
