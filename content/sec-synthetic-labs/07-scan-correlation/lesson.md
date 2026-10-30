---
title: Phase 3: correlate synthetic scan evidence
summary: Parse fictional Nmap-style logs and match exact service versions against a supplied advisory table.
minutes: 35
difficulty: medium
tags: security, synthetic-lab, scan-correlation
id: sec-synthetic-labs/scan-correlation
mode: harness
---

Reconnaissance converts observations into hypotheses. An open port does not prove a vulnerability; a banner may be stale or spoofed. In this lab, correlate exact service/version pairs with a fictional advisory table. Report candidates for manual validation, not confirmed exploitation.

Each scan record has four whitespace-separated columns: `<port>/tcp <state> <service> <version>`. Ignore headers, malformed records, UDP, closed and filtered ports. Advisory records use made-up LAB identifiers, not claims about real CVEs. Production triage also needs vendor patch status, configuration and authentication context.

Use Run to check compilation. Test supplies varied hidden inputs. All data and targets are fictional; nothing connects to an external service.

For the C control-flow exercise, complete [Redirect a modeled stack return](/learn/sec-c-bugs/stack-control). Return here for privilege and directory analysis.
