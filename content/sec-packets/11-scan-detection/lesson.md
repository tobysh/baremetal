---
title: Spot a port scan
summary: Summarise synthetic connection records per client and flag a source that probed many distinct service endpoints where most did not answer.
minutes: 30
difficulty: medium
tags: security, protocol-analysis, detection
id: sec-packets/scan-detection
mode: stdout
---
A port scan asks many different service endpoints, "are you there?", to map what a machine offers. A connection attempt to a TCP port gets one of three outcomes that an observer can record: it opens (the service answers), it is closed (the host answers with a reset) or it is filtered (nothing comes back, usually a firewall). A client that touches many distinct destination ports and mostly meets closed or filtered ones is behaving very differently from one that talks to the three ports it needs.

Flow records make this easy to see. Here each line is `time client server port outcome` for connection attempts seen by a monitor on a synthetic network, using documentation addresses. Count distinct targets, a target being the pair of server and port, and count each target once, using the first outcome recorded for it, because retries of the same target say nothing new about coverage. Flag a client only when it reached five or more distinct targets and more of them failed (closed or filtered) than opened.

This is triage over a bounded file, with no time window. A real detector would use windows, per-server thresholds, and exceptions for authorised vulnerability scanning, and an alert is an observation to investigate, not proof of intent. Ignore lines you cannot parse: comments starting with `#`, blank lines, ports outside 1 to 65535 or unknown outcomes.
