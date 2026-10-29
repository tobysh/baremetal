---
title: Correlate authentication failures
summary: Summarize password failures by source and flag the first later password acceptance after a threshold.
minutes: 25
difficulty: medium
tags: security, defensive-programming
id: sec-forensics/auth-log-analysis
mode: stdout
---
Authentication logs contain both useful events and noise. Parse the event grammar rather than counting every occurrence of the word Failed. This lesson considers only sshd password failures and password acceptances. Public-key events, connection notices and unrelated programs are ignored.

For a failure, extract the username after `Failed password for`, skipping an optional `invalid user`, and the address after `from`. Maintain a total and a set of distinct usernames for each source address. Mark an address as a candidate when it reaches five failures in this supplied file. If a password acceptance subsequently appears for that address, record the first accepted username. An acceptance before the fifth failure does not qualify.

Sort candidates by descending failure count, then lexicographic address text for ties. The text order is deliberate, not numeric IP order. This is a simple triage rule over a bounded file, with no sliding time window and no assertion that an address identifies a person. NAT, legitimate mistakes and forged/missing logs can all affect interpretation. The word ACCEPTED reports what the synthetic log says; it does not prove that the failures caused the later login.
