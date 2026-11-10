---
title: Spot a port scan
summary: Summarise synthetic connection records per client and flag a source that probed many distinct service endpoints where most did not answer.
minutes: 30
difficulty: medium
tags: security, protocol-analysis, detection
id: sec-packets/scan-detection
mode: stdout
---
A port scan asks many different service endpoints "are you there?" to map what a machine offers. From a monitor's side each attempt has one of three outcomes: `open` (the service answered), `closed` (the host replied with a reset) or `filtered` (nothing came back, usually a firewall). A client that touches three ports it needs looks very different from one that touches forty and meets mostly closed or filtered doors.

## Layout

```text
one record per line, five whitespace-separated fields:

  time   client         server         port  outcome
  1001   198.51.100.23  203.0.113.6    21    closed

target  = (server, port) pair            counted once per client
outcome = the FIRST one recorded for that target
flag a client when:  targets >= 5  and  closed + filtered > open

ignored lines: '#' comments, blank lines, wrong field count,
               port outside 1..65535, outcome not open/closed/filtered
```

The file is a bounded sample with no time window, using documentation addresses and invented servers.

## Worked example

```text
1000 198.51.100.7 203.0.113.5 22 closed
1001 198.51.100.7 203.0.113.5 23 filtered
1002 198.51.100.7 203.0.113.6 22 closed
1003 198.51.100.7 203.0.113.5 22 open      <- retry of the first target, ignored
1004 198.51.100.7 203.0.113.7 80 closed
1005 198.51.100.7 203.0.113.7 443 closed
1006 192.0.2.11   203.0.113.5 80 open
```

For 198.51.100.7 the five distinct targets are (.5, 22), (.5, 23), (.6, 22), (.7, 80) and (.7, 443). The retry on line four is the same target as line one, so the first outcome, `closed`, stays. The totals are open=0, closed=4, filtered=1, and since 5 >= 5 and 5 > 0 the output line is `198.51.100.7 targets=5 open=0 closed=4 filtered=1`. The client 192.0.2.11 has one target and is not listed. If nothing qualifies, print `no scans detected`.

## Pitfalls

- **Target, not port.** Port 22 on two servers is two targets. Port 22 on one server five times is one.
- **First outcome wins.** A retry that later succeeds does not rewrite the history. Search the list before adding.
- **Parse strictly.** A line with six fields, a port of 0 or 65536, or `reset` as an outcome is junk and must not be half-counted.
- **Sort order.** Count descending, then client name with `strcmp`, so output is deterministic.
- **Bounds.** Limits are 1000 lines, 256 clients and 256 targets per client; names are at most 63 characters. Do not let one client overflow its array.

## The defender's view

A scan count is a prompt to look, not a verdict. Authorised vulnerability scanners and monitoring checks behave exactly like this, shared addresses merge many people, and a slow scan stays under any threshold that has no time window. Real detectors add windows, per-destination rules and allow-lists, then alert a human. Treat the output as triage: ask who owns the address and whether the scan was expected before drawing any conclusion.
